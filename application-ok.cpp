#include "executive.h"
#include "busy_wait.h"

#define BUSY_WAIT_GENERATION
#define START_TASK
//#define DEBUG

Executive exec(5, 4);

void task0()
{
	#ifdef START_TASK
	std::cout << "- Task 0 inizio esecuzione" << std::endl;
	#endif

	#ifdef DEBUG
	auto start = std::chrono::high_resolution_clock::now();
	#endif

	#ifdef BUSY_WAIT_GENERATION
	busy_wait(78);
	#endif

	std::cout << "- Task 0 termina esecuzione" << std::endl;

	#ifdef DEBUG
	auto stop = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed(stop - start);
	std::cout << "-- Task 0	Elapsed [ms]: " << elapsed.count() << std::endl;
	#endif
}

void task1()
{
	#ifdef START_TASK
	std::cout << "- Task 1 inizio esecuzione" << std::endl;
	#endif

	#ifdef DEBUG
	auto start = std::chrono::high_resolution_clock::now();
	#endif

	#ifdef BUSY_WAIT_GENERATION
	busy_wait(18);
	#endif

	std::cout << "- Task 1 termina esecuzione" << std::endl;

	#ifdef DEBUG
	auto stop = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed(stop - start);
	std::cout << "-- Task 1	Elapsed [ms]: " << elapsed.count() << std::endl;
	#endif
}

void task2()
{
	#ifdef START_TASK
	std::cout << "- Task 2 inizio esecuzione" << std::endl;
	#endif

	#ifdef DEBUG
	auto start = std::chrono::high_resolution_clock::now();
	#endif

	#ifdef BUSY_WAIT_GENERATION
	busy_wait(9);
	#endif

	std::cout << "- Task 2 termina esecuzione" << std::endl;

	#ifdef DEBUG
	auto stop = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed(stop - start);
	std::cout << "-- Task 2	Elapsed [ms]: " << elapsed.count() << std::endl;
	#endif
}

void task3()
{
	#ifdef START_TASK
	std::cout << "- Task 3 inizio esecuzione" << std::endl;
	#endif

	#ifdef DEBUG
	auto start = std::chrono::high_resolution_clock::now();
	#endif

	#ifdef BUSY_WAIT_GENERATION
	busy_wait(9);
	#endif

	std::cout << "- Task 3 termina esecuzione" << std::endl;

	#ifdef DEBUG
	auto stop = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed(stop - start);
	std::cout << "-- Task 3	Elapsed [ms]: " << elapsed.count() << std::endl;
	#endif
}

void task4()
{
	#ifdef START_TASK
	std::cout << "- Task 4 inizio esecuzione" << std::endl;
	#endif

	#ifdef DEBUG
	auto start = std::chrono::high_resolution_clock::now();
	#endif

	#ifdef BUSY_WAIT_GENERATION
	busy_wait(9);
	#endif

	std::cout << "- Task 4 termina esecuzione" << std::endl;

	#ifdef DEBUG
	auto stop = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed(stop - start);
	std::cout << "-- Task 4	Elapsed [ms]: " << elapsed.count() << std::endl;
	#endif
}

/* Nota: nel codice di uno o piu' task periodici e' lecito chiamare Executive::ap_task_request() */

void ap_task()
{
	/* Custom Code */
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

	//exec.set_aperiodic_task(ap_task, 2);

	exec.add_frame({0,1,2});
	exec.add_frame({0,3});
	exec.add_frame({0,1});
	exec.add_frame({0,1});
	exec.add_frame({0,1,4});
	/* ... */

	exec.run();

	return 0;
}
