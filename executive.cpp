#include <cassert>
#include "executive.h"
#include <ctime>

#define DEBUG
#define SLACK_STEALING_ON //commentare per la versione senza slack stealing

Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{
}

void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet)
{
	//std::cout << "set_periodic_task" << std::endl;
	assert(task_id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)

	p_tasks[task_id].function = periodic_task;
	p_tasks[task_id].wcet = wcet * unit_time.count();
}

void Executive::set_aperiodic_task(std::function<void()> aperiodic_task, unsigned int wcet)
{
 	ap_task.function = aperiodic_task;
 	ap_task.wcet = wcet;
}

void Executive::add_frame(std::vector<size_t> frame)
{
	//std::cout << "add_frame" << std::endl;
	for (auto & id: frame)
		assert(id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)

	frames.push_back(frame);
	/* ... */
}

void Executive::run()
{
	rt::priority prio(rt::priority::rt_max-2);
	rt::affinity af("1");

	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id

		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]), std::ref(mutex));
		p_tasks[id].my_status = IDLE;
		p_tasks[id].index = id;
		/* ... */

		rt::set_affinity(p_tasks[id].thread, af);
		try {
			rt::set_priority(p_tasks[id].thread, prio);
		}
		catch(rt::permission_error & e) {
			std::cout << "Failed to set priority " << e.what() << std::endl;
		}
		--prio;
	}

	assert(ap_task.function); // Fallisce se set_aperiodic_task() non e' stato invocato
	ap_task.thread = std::thread(&Executive::task_function, std::ref(ap_task), std::ref(mutex));
	ap_task.my_status = IDLE;
	ap_task.index = APERIODIC;
	release_aperiodic = false;

	rt::set_affinity(ap_task.thread, af);

	try {
		#ifndef SLACK_STEALING_ON
			rt::set_priority(ap_task.thread, rt::priority::rt_min);
		#else
			rt::set_priority(ap_task.thread, rt::priority::rt_max-1);
		#endif
	}
	catch(rt::permission_error & e) {
		std::cout << "Failed to set priority " << e.what() << std::endl;
	}

	//imposto la priorità massima all'executive e l'affinity 1 (lavoreranno su un processore)
	std::thread exec_thread(&Executive::exec_function, this);
	rt::set_affinity(exec_thread, af);

	try
	{
		rt::set_priority(exec_thread, rt::priority::rt_max);
	}
	catch (rt::permission_error & e)
	{
		std::cerr << "Error setting RT priorities: " << e.what() << std::endl;
		exec_thread.detach();
	}

	/* ... */

	exec_thread.join();

	ap_task.thread.join();

	for (auto & pt: p_tasks)
		pt.thread.join();
}

void Executive::ap_task_request()
{
	/* ... */
	std::unique_lock<std::mutex> lock(mutex);
	release_aperiodic = true;
}

void Executive::task_function(Executive::task_data & task, std::mutex &mtx)
{
	while (true) {
		{
			std::unique_lock<std::mutex> l(mtx);

			while (task.my_status == IDLE)
				task.cond.wait(l);
				
			task.my_status = RUNNING;  // RUNNING : task in esecuzione
		}

		#ifdef DEBUG
			auto start = std::chrono::high_resolution_clock::now();
		#endif

		task.function();

		#ifdef DEBUG
			auto stop = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed(stop - start);
			if(task.index != APERIODIC)
				std::cout << "- Task " << task.index << " " << "Elapsed [ms]: " << elapsed.count() << std::endl;
			else
				std::cout << "- Task Aperiodico Elapsed [ms]: " << elapsed.count() << std::endl;
		#endif

		std::unique_lock<std::mutex> l(mtx); //cambio di stato in regione critica
		task.my_status = IDLE;				 // IDLE : in questo caso definisce il completamento dell'esecuzione

	}
}

void Executive::exec_function()
{
	unsigned int frame_id = 0;
	unsigned int slack_time = 0;

	rt::priority min_priority_frame(rt::priority::rt_min); //mi serve per ottenere il valore minimo di priorità all'interno dei task del frame

	/* ... */

	auto point = std::chrono::steady_clock::now();
	auto base_point = std::chrono::high_resolution_clock::now();;
	auto start = std::chrono::high_resolution_clock::now();

	while (true)
	{
		/* Rilascio dei task periodici del frame corrente e aperiodico (se necessario)... */
		//frame 0 istante 0 -> rilascio 0

		{
			std::unique_lock<std::mutex> lock(mutex); //non posso fidarmi solo della priorità massima dell'executive, utilizzo un mutex

			std::cout << "Frame " << frame_id << std::endl;
			
			//stampo i task previsti e calcolo lo slack time
			std::cout << "Previsti i task: ";
			for(unsigned int i = 0; i < frames[frame_id].size(); ++i){
				std::cout << "{" << frames[frame_id][i] << "} ";
				min_priority_frame = rt::get_priority(p_tasks[frames[frame_id][i]].thread) - 1; //l'ultimo valore che rimane è la priorità minima presente nel frame
				
				slack_time+=p_tasks[frames[frame_id][i]].wcet;
			}
			std::cout << std::endl;
			
			slack_time = frame_length * unit_time.count() - slack_time; //slacktime rimanente
			std::cout << "Slack Time: " << slack_time <<std::endl;

			//Scheduling Task Periodici:
			for(unsigned int i = 0; i < frames[frame_id].size(); ++i) {
				if(p_tasks[frames[frame_id][i]].my_status == IDLE){

					if (rt::get_priority(p_tasks[frames[frame_id][i]].thread) == rt::priority::rt_min + p_tasks.size() - frames[frame_id][i] ){
						try{
							rt::set_priority(p_tasks[frames[frame_id][i]].thread, rt::priority::rt_max-2-frames[frame_id][i]); //riacquisisco la priorità se è tornato in IDLE -> se ha finito
						}
						catch(rt::permission_error & e) {
							std::cout << "Failed to set priority " << e.what() << std::endl;
						}
					}

					auto checkpoint = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double, std::milli> elapsed(checkpoint - base_point);
					std::cout << "Thread: " << frames[frame_id][i] <<
					" -Release: " << elapsed.count() <<
					" -Priority: "<< rt::get_priority(p_tasks[frames[frame_id][i]].thread) << std::endl;

					p_tasks[frames[frame_id][i]].my_status = PENDING;			// Task pronto per l'esecuzione
					p_tasks[frames[frame_id][i]].cond.notify_one();				// Notifico il task
				}
			}

			//Scheduling Task Aperiodico:
			if (ap_task.my_status == IDLE && slack_time > 0 && release_aperiodic) {

				auto checkpoint = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> elapsed(checkpoint - base_point);
				std::cout << "Thread Aperiodico" <<
				" -Release: " << elapsed.count() <<
				" -Priority: "<< rt::get_priority(ap_task.thread) << std::endl;

				ap_task.my_status = PENDING;
				ap_task.cond.notify_one();
				release_aperiodic = false;
			}
		}

		/* Attesa fino al prossimo inizio frame ... */

		#ifndef SLACK_STEALING_ON
			point += std::chrono::milliseconds(frame_length * unit_time); //imposta ogni quanti millisecondisecondi deve ripetersi
			std::this_thread::sleep_until(point);			// l'executive va in sleep per tutta la durata effettiva del frame, temporizzazione in modo assoluto
		#else
			if (slack_time > 0 && (ap_task.my_status == PENDING || ap_task.my_status == RUNNING)){// && rt::get_priority(ap_task.thread) != rt::priority::rt_min+p_tasks.size()+1){
				//entro qui qualora ci sia slack_time disponibile, il task sia stato notificato //e se non ha avuto una deadline

				//per questo quanto di tempo la priorità è massima rispetto a tutti gli altri task
				try {
					rt::set_priority(ap_task.thread, rt::priority::rt_max-1);
				}
				catch(rt::permission_error & e) {
					std::cout << "Failed to set priority " << e.what() << std::endl;
				}
				
				//partiziono la priorità dell'aperiodico all'interno del frame in modo da dedicare (slack_time)-quanti di tempo al task aperiodico.
				//NB slack_time tiene già conto dello unit_time

				point += std::chrono::milliseconds(slack_time); //imposta ogni quanti millisecondisecondi deve ripetersi
				std::this_thread::sleep_until(point);			// l'executive va in sleep per tutta la durata effettiva del frame, temporizzazione in modo assoluto

				//allo scadere del partizionamento in cui l'aperiodico ha priorità massima, pongo una priorità inferiore rispetto al frame residuo
				try {
					rt::set_priority(ap_task.thread, rt::priority::rt_min);
				}
				catch(rt::permission_error & e) {
					std::cout << "Failed to set priority " << e.what() << std::endl; 
				}

				//sleep per tutto il tempo residuo
				point += std::chrono::milliseconds((frame_length* unit_time.count()) - slack_time);
				std::this_thread::sleep_until(point);
			}
			else{
				//qualora non ci siano ap_task in ballo, non c'è bisogno di partizionamento delle priorità
				point += std::chrono::milliseconds(frame_length * unit_time); //imposta ogni quanti millisecondisecondi deve ripetersi
				std::this_thread::sleep_until(point);			// l'executive va in sleep per tutta la durata effettiva del frame, temporizzazione in modo assoluto
			}
		#endif

		/* Controllo delle deadline ... */
		{
			std::unique_lock<std::mutex> lock(mutex);

			std::cout << "-> Controllo rispetto deadline nel frame precedente..." << std::endl;
			//Controllo Deadline task periodici
			for(unsigned int i = 0; i < frames[frame_id].size(); ++i) {
				if(p_tasks[frames[frame_id][i]].my_status == RUNNING || p_tasks[frames[frame_id][i]].my_status == PENDING){ //controllo se i task hanno finito nel frame precedente
					
					if(p_tasks[frames[frame_id][i]].my_status == RUNNING)
						std::cout << "   Task " << frames[frame_id][i] <<" Deadline Miss!" << std::endl;

					else if(p_tasks[frames[frame_id][i]].my_status == PENDING)
						std::cout << "   Task " << frames[frame_id][i] <<" Deadline Miss! (Il task non è stato eseguito)" << std::endl;
					
					try {
						rt::set_priority(p_tasks[frames[frame_id][i]].thread, rt::priority::rt_min + p_tasks.size() - frames[frame_id][i]);
						//I task con priorità rt::priority::rt_min + frame_length - frames[frame_id][i] hanno avuto una deadline miss
					}
					catch(rt::permission_error & e) {
						std::cout << "Failed to set priority " << e.what() << std::endl;
					}
				}
			}

			//Controllo Deadline task aperiodico
			if (release_aperiodic && ap_task.my_status == RUNNING){
				{
					std::unique_lock<std::mutex> lock(mutex);
					release_aperiodic = false;
				}
				std::cout << "   Task Aperiodico Deadline Miss!" << std::endl;
			}
		}

		if (++frame_id == frames.size()){
			frame_id = 0;
		}
		slack_time = 0;

		#ifdef DEBUG
			auto stop = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed(stop - start);
			start = stop;
			std::cout << "--- Frame latency [ms]: " << elapsed.count()<< std::endl;
			std::cout << std::endl;
		#endif
	}
}