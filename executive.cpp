#include <cassert>

#include "executive.h"
#include <ctime>

#define DEBUG

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
	p_tasks[task_id].wcet = wcet;
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
	rt::priority prio(rt::priority::rt_max-1);
	rt::affinity af("1");

	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id

		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]));

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

	//assert(ap_task.function); // Fallisce se set_aperiodic_task() non e' stato invocato

	//ap_task.thread = std::thread(&Executive::task_function, std::ref(ap_task));

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

	//ap_task.thread.join();

	for (auto & pt: p_tasks)
		pt.thread.join();
}

void Executive::ap_task_request()
{
	/* ... */
}

void Executive::task_function(Executive::task_data & task)
{
	std::unique_lock<std::mutex> l(task.mutex);
	task.my_status = IDLE;								// IDLE : task non ancora pronto

	while (true) {
		task.my_status = PENDING;						// PENDING : task pronto per l'esecuzione
		// wait for activation
		while (task.my_status == PENDING)
			task.cond.wait(l);
		task.function();								// RUNNING : task in esecuzione

		task.my_status = IDLE;							// IDLE : task non ancora pronto

	}

}

void Executive::exec_function()
{
	unsigned int frame_id = 0;
	/* ... */

	//imposto il clock dell'executive
	auto point = std::chrono::steady_clock::now();
	auto base_point = std::chrono::high_resolution_clock::now();;

	while (true)
	{

		#ifdef DEBUG
		auto start = std::chrono::high_resolution_clock::now();
		#endif


		/* Rilascio dei task periodici del frame corrente e aperiodico (se necessario)... */
		//frame 0 istante 0 -> rilascio 0

		std::cout << "Frame " << frame_id << ":" << std::endl;
		for(unsigned int i = 0; i < frames[frame_id].size(); ++i) {
			if(p_tasks[frames[frame_id][i]].my_status == PENDING){
				
				auto checkpoint = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> elapsed(checkpoint - base_point);
				std::cout << "Thread " << frames[frame_id][i] << " - Release: " << elapsed.count()<< std::endl;

				p_tasks[frames[frame_id][i]].my_status = RUNNING;			// Metto il task nello stato di RUNNING
				p_tasks[frames[frame_id][i]].cond.notify_one();				// Notifico il task

			}

		}


		/* Attesa fino al prossimo inizio frame ... */
		point += std::chrono::milliseconds(frame_length * unit_time); //imposta ogni quanti millisecondisecondi deve ripetersi
		std::this_thread::sleep_until(point);			// l'executive va in sleep per tutta la durata effettiva del frame, temporizzazione in modo assoluto


		/* Controllo delle deadline ... */


		if (++frame_id == frames.size())
			frame_id = 0;
		
		std::cout << "-> Controllo rispetto deadline nel frame precedente:" << std::endl;

		for(unsigned int i = 0; i < frames[frame_id].size(); ++i) {
			if(p_tasks[frames[frame_id][i]].my_status == RUNNING){ //controllo se i task hanno finito nel frame precedente
				//p_tasks[frames[frame_id][i]].my_status = IDLE;
				std::cout << "   Task " << frames[frame_id][i] <<" Deadline Miss: " << std::endl;

				try {
					rt::set_priority(p_tasks[frames[frame_id][i]].thread, rt::priority::rt_min); //setto la priorità del task con deadline miss al minimo in modo da non intaccare l'esecuzione degli altri task
				}
				catch(rt::permission_error & e) {
					std::cout << "Failed to set priority " << e.what() << std::endl;
				}

			}
		}

		#ifdef DEBUG
		auto stop = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed(stop - start);
		std::cout << "--- Frame latency [ms]: " << elapsed.count()<< std::endl;
		std::cout << std::endl;
		#endif

	}
}
