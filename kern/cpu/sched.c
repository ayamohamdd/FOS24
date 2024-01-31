#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);
//	sched_init_BSD(64, INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");
	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{

		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{

			enqueue(&(env_ready_queues[0]), curenv);
		}


		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again

	struct Env* old_curenv = curenv;

	curenv = next_env ;

	chk2(next_env) ;

	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{

		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);

	}
	else
	{

		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP
	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	num_of_ready_queues=numOfLevels;
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue)*num_of_ready_queues);
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
	for(int i=PRI_MIN;i<num_of_ready_queues;i++)
	{
		    quantums[i] = quantum;
			kclock_set_quantum(quantums[i]);
			init_queue(&(env_ready_queues[i]));
	}
	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}


//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	//return NULL;
	struct Env* next_env=NULL;

	//[1] Place the curenv (if any) in its correct queue

	if(curenv!=NULL) { //(if any)
		//place				 curenv in its correct queue
		enqueue(&env_ready_queues[curenv->priority],curenv);
	}

	//===========================================================//
	//===========================================================//

	//[2] Search for the next env in the queues according to their priorities
	for(int i=num_of_ready_queues-1 ;i >= 0; i--){
		if(env_ready_queues[i].size > 0 /*&& LIST_LAST(&env_ready_queues[i]) != NULL*/ /*BUG*/) {

			//[3] If the next env is found:

			//		1. Set the CPU quantum
			kclock_set_quantum(quantums[i]);

			//		2. Remove the selected env from its queue and return it
			next_env=dequeue(&env_ready_queues[i]);
			return next_env;
		}

	}
	//[3] CONTs....
	// Else:
	//	1. Reset load_avg for next run
	loadAVG = fix_int(0);
	//	2. return NULL
	return NULL;

}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	{

//		cprintf("BEFOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOORE\n");
//		sched_print_all();
//		cprintf("CLOCKER %u\n", quantums[0] * ticks);
		//ready_processes: number of processes that are either ruuning or ready at time of update.



		//loadAVG: estimates the average number of ready processes over the past minute.
		//It is initialized to 0 at boot and recalculated once per second
		/*recalculated once per second -> tick = 1quantum ms, 1s = 1000ms*/
		if((quantums[0] * ticks)%1000 == 0) {
			int ready_processes = (curenv != NULL); //either ruuning
				for(int i = PRI_MIN; i < num_of_ready_queues; i++) {//or ready
					if(LIST_SIZE(&env_ready_queues[i]) > 0)
						ready_processes += LIST_SIZE(&env_ready_queues[i]);
				}
			//loadAVG(t) = 59/60 * loadAVG(t-1) + 1/60 * ready_processes(t) -> 1/60 * 10 -> 1/6
			/*
			 * x1 = 59/60
			 * x = x1 * loadAVG(t-1) -> 59/60 * loadAVG(t-1)
			 * y1 = 1/60
			 * y = y1 * ready_processes(t) -> 1/60 * ready_processes(t)
			 * loadAVG(t) = x + y -> 59/60 * loadAVG(t-1) + 1/60 * ready_processes(t)-> 1/60 * 3
			 * 59/60 * 3/60 + 1/60 * 3
			 */
			fixed_point_t x1 = fix_frac(59, 60);				// 59/60
			fixed_point_t x = fix_mul(x1, loadAVG);				// 59/60 * loadAVG(t-1)
			fixed_point_t y1 = fix_frac(1, 60);					// 1/60
			fixed_point_t y = fix_scale(y1, ready_processes);	// 1/60 * ready_processes(t)
			loadAVG = fix_add(x, y);							// 59/60 * loadAVG(t-1) + 1/60 * ready_processes(t)
//			cprintf("ELLOADAVG = %u \n", (loadAVG));
		}

//		//recentCPU: measures the amount of CPU time a process has received "recently".
//		//1. On each timer tick(quantum): recentCPU of running process's is incremented by 1.
		if(curenv != NULL)
			curenv->recentCPU = fix_add(curenv->recentCPU, fix_int(1));  // 1
//		//2. Once per second: recentCPU of every process's is updated.
		if((quantums[0] * ticks)%1000 == 0) {
			for(int i = 0; i < num_of_ready_queues; i++) {
				struct Env* updateEnv;
				LIST_FOREACH(updateEnv, &env_ready_queues[i]) {
					//recentCPU(t) = 2*loadAVG/(2*loadAVG +1) * recentCPU(t-1) + nice
					/*
					 * x1 = 2*loadAVG
					 * x2 = x1 + one -> (2*loadAVG +1)
					 * x3 = x1 / x2  -> 2*loadAVG/(2*loadAVG +1)
					 * x = x3 * recentCPU(t-1) -> 2*loadAVG/(2*loadAVG +1) * recentCPU(t-1)
					 * recentCPU = x3 + nice -> 2*loadAVG/(2*loadAVG +1) * recentCPU(t-1) + nice
					 */
//					cprintf("BEFORE RECENT CPU = %d \n", fix_trunc(updateEnv->recentCPU));
					fixed_point_t one = fix_int(1); 		  				    //ONE IN FIXED
					fixed_point_t x1 = fix_scale(loadAVG, 2); 			        //2*loadAVG
					fixed_point_t x2 = fix_add(x1, one); 	  				    //(2*loadAVG +1)
					fixed_point_t x3 = fix_div(x1, x2);	      				    //2*loadAVG / (2*loadAVG +1)
					fixed_point_t x = fix_mul(x3, updateEnv->recentCPU);       //2*loadAVG / (2*loadAVG +1) * recentCPU
//					cprintf("X BKAM! %d \n", x);
					updateEnv->recentCPU = fix_add(x, fix_int(updateEnv->nice));//2*loadAVG/(2*loadAVG +1)*recentCPU+nice
//					cprintf("RECENT CPU = %d \n", fix_trunc(updateEnv->recentCPU));

				}
			}

//					cprintf("BEFORE RECENT CPU = %d \n", fix_trunc(updateEnv->recentCPU));
					fixed_point_t one = fix_int(1); 		  				    //ONE IN FIXED
					fixed_point_t x1 = fix_scale(loadAVG, 2); 			        //2*loadAVG
					fixed_point_t x2 = fix_add(x1, one); 	  				    //(2*loadAVG +1)
					fixed_point_t x3 = fix_div(x1, x2);	      				    //2*loadAVG / (2*loadAVG +1)
					fixed_point_t x = fix_mul(x3, curenv->recentCPU);       //2*loadAVG / (2*loadAVG +1) * recentCPU
//					cprintf("X BKAM! %d \n", x);
					curenv->recentCPU = fix_add(x, fix_int(curenv->nice));//2*loadAVG/(2*loadAVG +1)*recentCPU+nice
//					cprintf("RECENT CPU = %d \n", fix_trunc(updateEnv->recentCPU));

		}

//		//Priority: between 0 (PRI_MIN) through 63 (PRI_MAX), which is recalculated every 4th tick.
		/*recalculated every 4th tick*/
		if(ticks%4 == 0) {
			//priority = PRI_MAX - (recentCPU/4) - (nice * 2) -> 63 - 1/4 -> 22
			/*
			 * x1 = PRI_MAX
			 * x2 = recentCPU
			 * x3 = nice * 2
			 * y1 = x1 - x2
			 * res = y1 - x3
			 * priority = res rounded down to the nearest integer
			 */

//====================================================================================//
//									RUNNIG PROCESS (curenv)
//====================================================================================//
			if(curenv != NULL) {
			fixed_point_t x1 = fix_int(num_of_ready_queues-1); 	// PRI_MAX   FIXED
			fixed_point_t x2 = curenv->recentCPU;			  	// recentCPU FIXED
			x2 = fix_unscale(x2, 4);							// recentCPU / 4
			fixed_point_t x3 = fix_int(curenv->nice);		    // nice 	 FIXED
			x3 = fix_scale(x3, 2);								// nice * 2
			fixed_point_t y1 = fix_sub(x1, x2);					// PRI_MAX - (recentCPU/4)
			fixed_point_t res = fix_sub(y1, x3);				// PRI_MAX - (recentCPU/4) - (nice * 2)
			int p = fix_trunc(res);
			if(p > num_of_ready_queues-1) p = num_of_ready_queues-1;
			else if(p < PRI_MIN) p = PRI_MIN;
			int oldp = curenv->priority;
			curenv->priority = p;
//			cprintf("ELRUNNING = %d \n", curenv->priority);
//			if(oldp != p) {
//					remove_from_queue(&env_ready_queues[oldp], curenv);
//					enqueue(&env_ready_queues[curenv->priority], curenv);
//				}
			}

//====================================================================================//
//									 READY PROCESSES
//====================================================================================//
		for(int i = num_of_ready_queues - 1; i >= PRI_MIN; i--) {
			struct Env* updateEnv;
			LIST_FOREACH(updateEnv, &env_ready_queues[i]) {
				//priority = PRI_MAX - (recentCPU/4) - (nice * 2)
				/*
				 * x1 = PRI_MAX
				 * x2 = recentCPU
				 * x3 = nice * 2
				 * y1 = x1 - x2
				 * res = y1 - x3
				 * priority = res rounded down to the nearest integer
				 */

				fixed_point_t x1 = fix_int(num_of_ready_queues-1); 				// PRI_MAX   FIXED
				fixed_point_t x2 = updateEnv->recentCPU;			 	// recentCPU FIXED
				x2 = fix_unscale(x2, 4);							// recentCPU / 4
				fixed_point_t x3 = fix_int(updateEnv->nice);		// nice 	 FIXED
				x3 = fix_scale(x3, 2);								// nice * 2
				fixed_point_t y1 = fix_sub(x1, x2);					// PRI_MAX - (recentCPU/4)
//				cprintf("ABL ELNICE %d \n", fix_trunc(y1));
				fixed_point_t res = fix_sub(y1, x3);				// PRI_MAX - (recentCPU/4) - (nice * 2)
				int p = fix_trunc(res);
//				cprintf("EL P BEFORE %d\n", p);
				if(p > num_of_ready_queues - 1) p = num_of_ready_queues - 1;
				else if(p < PRI_MIN) p = PRI_MIN;
				int oldp = updateEnv->priority;
				updateEnv->priority = p;
//				cprintf("oldp %d vs %d p \n", oldp, p);

			}
		}

		for(int i = num_of_ready_queues - 1; i >= PRI_MIN; i--) {
			struct Env* updateEnv;
			LIST_FOREACH(updateEnv, &env_ready_queues[i]) {

				if(updateEnv->priority != i) {
					remove_from_queue(&env_ready_queues[i], updateEnv);
					enqueue(&env_ready_queues[updateEnv->priority], updateEnv);
				}

			}
		}
	}
}


//	cprintf("BEFOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOORE\n");
//	sched_print_all();

	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}
//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}

