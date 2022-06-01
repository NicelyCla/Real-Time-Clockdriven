#include "executive.h"
#include "busy_wait.h"

#define BUSY_WAIT_GENERATION

Executive exec(5, 4);

std::random_device rd;							// inizializzazione
std::mt19937 gen(rd());							// generatore
std::uniform_int_distribution<> dis(0, 4);		// random
unsigned int rand_gen = (int) dis(gen);			//
unsigned int count = 0;							//


void task0()
{
	std::cout << "- Task 0 inizio esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	#ifdef BUSY_WAIT_GENERATION
		busy_wait(8);
	#endif

	std::cout << "- Task 0 termina esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	if(++count % 6 == 0) {						// ap_task lanciato ogni 5 esecuzioni di task 0;
	//if(++count == rand_gen) {						// ap_ task lanciato in modo randomico;
		std::cout << "	Lancio ap_task_request() in modo random " << std::endl;
		exec.ap_task_request();
		rand_gen = dis(gen);						// random cambia ad ogni esecuzione di ap_task;
		count = 0;
	}
}

void task1()
{
	std::cout << "- Task 1 inizio esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	#ifdef BUSY_WAIT_GENERATION
		busy_wait(16);
	#endif

	std::cout << "- Task 1 termina esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;
}

void task2()
{
	std::cout << "- Task 2 inizio esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	#ifdef BUSY_WAIT_GENERATION
		busy_wait(8);
	#endif

	std::cout << "- Task 2 termina esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;
}

void task3()
{
	std::cout << "- Task 3 inizio esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	#ifdef BUSY_WAIT_GENERATION
		busy_wait(8);
	#endif

	std::cout << "- Task 3 termina esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;
}

void task4()
{
	std::cout << "- Task 4 inizio esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	#ifdef BUSY_WAIT_GENERATION
		busy_wait(8);
	#endif

	std::cout << "- Task 4 termina esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;
}

/* Nota: nel codice di uno o piu' task periodici e' lecito chiamare Executive::ap_task_request() */

void ap_task()
{
	std::cout << "- Task Aperiodico inizio esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;

	#ifdef BUSY_WAIT_GENERATION
		busy_wait(16);
	#endif

	std::cout << "- Task Aperiodico termina esecuzione [Prio: "<< rt::this_thread::get_priority() <<"]" << std::endl;
}

int main()
{
	busy_wait_init();
	// wcet= tempo massimo di esecuzione previsto
	exec.set_periodic_task(0, task0, 1); // tau_1
	exec.set_periodic_task(1, task1, 2); // tau_2
	exec.set_periodic_task(2, task2, 1); // tau_3,1
	exec.set_periodic_task(3, task3, 3); // tau_3,2
	exec.set_periodic_task(4, task4, 1); // tau_3,3
	
	/* ... */

	exec.set_aperiodic_task(ap_task, 2);

	exec.add_frame({0,1,2});
	exec.add_frame({0,3});
	exec.add_frame({0,1});
	exec.add_frame({0,1});
	exec.add_frame({0,1,4});
	/* ... */

	exec.run();

	return 0;
}
