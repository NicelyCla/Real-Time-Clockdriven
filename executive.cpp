#include <cassert>

#include "executive.h"
#include <ctime>

#define DEBUG
//#define SLACK_STEALING_ON

Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{
}

void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet)
{
	//Prima funzione eseguita
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
	//Seconda funzione eseguita
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
	//metti un mutex
	if (ap_task.my_status == IDLE){
		//qua serve un mutex
		release_aperiodic = true;
	}

	else if(ap_task.my_status == RUNNING){

		//Deadline miss task aperiodico -> priorità a 0
		try {

			#ifndef SLACK_STEALING_ON
				rt::set_priority(ap_task.thread, rt::priority::rt_min-1);
			#else
				rt::set_priority(ap_task.thread, rt::priority::rt_max-1);
			#endif

		}
		catch(rt::permission_error & e) {
			std::cout << "Failed to set priority " << e.what() << std::endl;
		}
	}

}

void Executive::task_function(Executive::task_data & task, std::mutex &mtx)
{
	while (true) {
		{
			std::unique_lock<std::mutex> l(mtx);
			//task.my_status = IDLE;							// IDLE : task non ancora pronto

			while (task.my_status == IDLE)
				task.cond.wait(l);
				
			task.my_status = RUNNING;

		}

		//std::cout << " Esecuzione n: " << count << std::endl;
		/*
		mtx.lock();
		task.my_status = RUNNING;
		mtx.unlock();
		*/
		task.function();								// RUNNING : task in esecuzione
		std::unique_lock<std::mutex> l(mtx); //cambio di stato in regione critica
		task.my_status = IDLE;							// IDLE : in questo caso definisce il completamento dell'esecuzione

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
			std::cout << "Previsti i task: ";
			for(unsigned int i = 0; i < frames[frame_id].size(); ++i){
				std::cout << "{" << frames[frame_id][i] << "} ";
				min_priority_frame = rt::get_priority(p_tasks[frames[frame_id][i]].thread) - 1; //l'ultimo valore che rimane è la priorità minima presente nel frame

			}
			std::cout << std::endl;

			//Scheduling Task Periodici:
			for(unsigned int i = 0; i < frames[frame_id].size(); ++i) {
				if(p_tasks[frames[frame_id][i]].my_status == IDLE){

					if (rt::get_priority(p_tasks[frames[frame_id][i]].thread) == rt::priority::rt_min + p_tasks.size() - frames[frame_id][i] ){
						rt::set_priority(p_tasks[frames[frame_id][i]].thread, rt::priority::rt_max-2-frames[frame_id][i]); //riacquisisco la priorità se è tornato in IDLE -> se ha finito
					}

					slack_time+=p_tasks[frames[frame_id][i]].wcet;

					auto checkpoint = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double, std::milli> elapsed(checkpoint - base_point);
					std::cout << "Thread: " << frames[frame_id][i] <<
					" -Release: " << elapsed.count() <<
					" -Priority: "<< rt::get_priority(p_tasks[frames[frame_id][i]].thread) << std::endl;
					//std::cout << "    WCETTONE NAZIONALE: " << p_tasks[frames[frame_id][i]].wcet <<std::endl;

					p_tasks[frames[frame_id][i]].my_status = PENDING;			// Task pronto per l'esecuzione
					p_tasks[frames[frame_id][i]].cond.notify_one();				// Notifico il task
					//p_tasks[frames[frame_id][i]].my_status = RUNNING;

					//std::cout << "La priorità del task " << frames[frame_id][i] << ": " << rt::get_priority(p_tasks[frames[frame_id][i]].thread) <<std::endl;
				}
			}
			slack_time = frame_length * unit_time.count() - slack_time; //slacktime rimanente

			//Scheduling Task Aperiodico:
			if (ap_task.my_status == IDLE && slack_time > 0 && release_aperiodic) {
				#ifndef SLACK_STEALING_ON
					if (rt::get_priority(ap_task.thread) == rt::priority::rt_min-1){
						rt::set_priority(ap_task.thread, rt::priority::rt_min);
					}
				#else
					std::cout<<"gestione da fare";
				#endif


				//rt::set_priority(ap_task.thread, min_priority_frame); //riacquisisco la priorità se è tornato in IDLE -> se ha finito

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
		std::cout << "slaccone nazionale: " << slack_time <<std::endl;


		/* Attesa fino al prossimo inizio frame ... */
		point += std::chrono::milliseconds(frame_length * unit_time); //imposta ogni quanti millisecondisecondi deve ripetersi
		std::this_thread::sleep_until(point);			// l'executive va in sleep per tutta la durata effettiva del frame, temporizzazione in modo assoluto

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
						//std::cout<<"aaaaaaaaaaa: "<< rt::priority::rt_min + p_tasks.size() - frames[frame_id][i] <<std::endl;
						//I task con priorità rt::priority::rt_min + frame_length - frames[frame_id][i] hanno avuto una deadline miss
					}
					catch(rt::permission_error & e) {
						std::cout << "Failed to set priority " << e.what() << std::endl;
					}

				}
			}

			//Controllo Deadline task aperiodico
			if (rt::get_priority(ap_task.thread) == rt::priority::rt_min-1){
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
