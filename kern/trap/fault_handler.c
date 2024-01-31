/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif



		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		if(isPageReplacmentAlgorithmFIFO()) {


			if(wsSize < (curenv->page_WS_max_size))
				{
				// FIFO placement logic

			//		cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
					//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
					// Write your code here, remove the panic and write your code
			//		panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
					//refer to the project presentation and documentation for details
					//1. Allocate space for the faulted page
				        struct FrameInfo* ptr_frame;
						allocate_frame(&ptr_frame);

						int perm = PERM_PRESENT |PERM_USER | PERM_WRITEABLE;
						map_frame(curenv->env_page_directory, ptr_frame, fault_va, perm);
						//

						//2. Read the faulted page from page file to memory
						int ret = pf_read_env_page(curenv, (void*)fault_va);

						//3. If the page does not exist on page file, then
						if(ret == E_PAGE_NOT_EXIST_IN_PF) {
						    //1. If it is a stack or a heap page, then, its OK.
						    if(( fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) ||
						    (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)) {
							//OK

						    }

						    //2. Else, it must be rejected without harm to the kernel or other running processes, by killing the process.
						    else {
						    sched_kill_env(curenv->env_id);

						    }
						}
						//4. Reflect the changes in the page working set list (i.e. add new element to list & update its last one)
						struct WorkingSetElement* wse = env_page_ws_list_create_element(curenv, fault_va);
						ptr_frame->element = wse;//bonus0
						LIST_INSERT_TAIL(&(curenv->page_WS_list), wse);
						if (LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size)
						{
							curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
						}
						else
						{
							curenv->page_last_WS_element = NULL;
						}

				}
			else {
			    // FIFO replacement logic
			    struct WorkingSetElement* element_to_remove = curenv->page_last_WS_element;

			    int i = pt_get_page_permissions(curenv->env_page_directory, element_to_remove->virtual_address);
			    if (i & PERM_MODIFIED) {
			        uint32 *ptr_page_table;
			        struct FrameInfo *removed_frame = get_frame_info(curenv->env_page_directory, element_to_remove->virtual_address, &ptr_page_table);
			        pf_update_env_page(curenv, element_to_remove->virtual_address, removed_frame);
			    }
		        // ws last is next
			    env_page_ws_invalidate(curenv, element_to_remove->virtual_address);

			    if (element_to_remove == LIST_LAST(&(curenv->page_WS_list))) {
			        // ws last is first
			        curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			    }

			    unmap_frame(curenv->env_page_directory, element_to_remove->virtual_address);

			    // Allocate
			    struct FrameInfo* new_frame;
			    allocate_frame(&new_frame);
			    int perm = PERM_PRESENT | PERM_USER | PERM_WRITEABLE;
			    map_frame(curenv->env_page_directory, new_frame, fault_va, perm);
			    //Read the faulted page from page file to memory
			    int ret = pf_read_env_page(curenv, (void*)fault_va);
			    //If the page does not exist on page file, then
				if(ret == E_PAGE_NOT_EXIST_IN_PF) {
				    //1. If it is a stack or a heap page, then, its OK.
				    if(( fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) ||
				    (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)) {
					//OK

				    }

				    //2. Else, it must be rejected without harm to the kernel or other running processes, by killing the process.
				    else {
				    sched_kill_env(curenv->env_id);

				    }
				}

			    struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, fault_va);
			    new_frame->element = new_element;

			    // insert at the tail
			    if (curenv->page_last_WS_element == LIST_FIRST(&(curenv->page_WS_list))) {
			        LIST_INSERT_TAIL(&(curenv->page_WS_list), new_element);
			    } else { //insert before the last element
			        LIST_INSERT_BEFORE(&(curenv->page_WS_list), curenv->page_last_WS_element, new_element);
			    }

			    // FIFO order after user mem
			    struct WorkingSetElement* tmp = LIST_FIRST(&(curenv->page_WS_list));
			    while (tmp != curenv->page_last_WS_element) {
			    	struct WorkingSetElement* tmp_next=LIST_NEXT(tmp);
			    	LIST_REMOVE(&(curenv->page_WS_list), tmp);
			    	LIST_INSERT_TAIL(&(curenv->page_WS_list),tmp);
			    	tmp =tmp_next;
			    }
			}

		    }
		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
			fault_va = ROUNDDOWN(fault_va,PAGE_SIZE);
			int Active_size=LIST_SIZE(&(curenv->ActiveList));
			//cprintf("Active_size %d ",Active_size);
			int Second_size=LIST_SIZE(&(curenv->SecondList));
			//cprintf("Second_size %d ",Second_size);
			if (Active_size+Second_size<(curenv->ActiveListSize+curenv->SecondListSize))
			{   //placement
				if (Active_size!=curenv->ActiveListSize&&Second_size==0)
				{
					struct FrameInfo* frame;
					allocate_frame(&frame);

					map_frame(curenv->env_page_directory, frame, fault_va,PERM_PRESENT |PERM_USER | PERM_WRITEABLE );
					int ret= pf_read_env_page(curenv, (void *)fault_va);
					if (ret == E_PAGE_NOT_EXIST_IN_PF)
					{
						if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
								||(fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))
						{

							  //do no thing
						}
						else
						{
						   sched_kill_env(curenv->env_id);
						}
					 }
					struct WorkingSetElement* Ae = env_page_ws_list_create_element(curenv, fault_va);
					LIST_INSERT_HEAD(&(curenv->ActiveList), Ae);
				}
				else if (Active_size==curenv->ActiveListSize)
				{
					if (Second_size!=curenv->SecondListSize)
					{
						//check if second size >0 and condition if in second size
						int in_secondlist=0;
						if (Second_size>0)
						{
							//loop on second list to check if fault va == it->virtual address
							struct WorkingSetElement *it;
							struct WorkingSetElement *founded=NULL;
							LIST_FOREACH(it, &(curenv->SecondList))
							{
							  //write your code.
								if ((uint32)it->virtual_address==fault_va)
								{
									in_secondlist=1;
									founded=it;
									LIST_REMOVE(&(curenv->SecondList),it);
									break;
								}
							}
							if (in_secondlist==1&&founded!=NULL)
							{
								pt_set_page_permissions(curenv->env_page_directory, founded->virtual_address,PERM_PRESENT,0);
								struct WorkingSetElement* element = LIST_LAST(&(curenv->ActiveList));
								LIST_REMOVE(&(curenv->ActiveList),element);
								LIST_INSERT_HEAD(&(curenv->ActiveList), founded);
								pt_set_page_permissions(curenv->env_page_directory, element->virtual_address, 0, PERM_PRESENT);
								LIST_INSERT_HEAD(&(curenv->SecondList), element);
								return ;
							}

						}
						struct FrameInfo* frame;
						allocate_frame(&frame);
						map_frame(curenv->env_page_directory, frame, fault_va,PERM_PRESENT |PERM_USER | PERM_WRITEABLE );
						int ret= pf_read_env_page(curenv, (void *)fault_va);
						if (ret == E_PAGE_NOT_EXIST_IN_PF)
						{
							if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
									||(fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))
							{
								  //do no thing
							}
							else
							{
							   sched_kill_env(curenv->env_id);
							}
						 }
						struct WorkingSetElement* Ae = env_page_ws_list_create_element(curenv, fault_va);
						//LIST_INSERT_HEAD(&(curenv->ActiveList), Ae);
						struct WorkingSetElement* element = LIST_LAST(&(curenv->ActiveList));
						LIST_REMOVE(&(curenv->ActiveList),element);
						LIST_INSERT_HEAD(&(curenv->ActiveList), Ae);
						pt_set_page_permissions(curenv->env_page_directory, element->virtual_address, 0, PERM_PRESENT);
						LIST_INSERT_HEAD(&(curenv->SecondList), element);
					}
				}

			}else{
				//TODO: [PROJECT'23.MS3 - #3] [1] PAGE FAULT HANDLER - LRU Replacement
				// Write your code here, remove the panic and write your code

				struct WorkingSetElement* new_wse = NULL;
				struct WorkingSetElement* temp_victim_head =NULL;
				struct WorkingSetElement* temp_active_head =LIST_FIRST(&(curenv->ActiveList));
				int inVictim = 0;

				LIST_FOREACH(temp_victim_head,&(curenv->SecondList)){
					if(fault_va ==temp_victim_head->virtual_address){
						inVictim = 1;
						break;
					}
				}

				 if(inVictim){
					LIST_REMOVE(&(curenv->SecondList),temp_victim_head);
					struct WorkingSetElement* temp_tail_active  = LIST_LAST(&(curenv->ActiveList));
					struct WorkingSetElement* tail_active  = LIST_LAST(&(curenv->ActiveList));

					LIST_REMOVE(&(curenv->ActiveList),tail_active);
					pt_set_page_permissions(curenv->env_page_directory,temp_tail_active->virtual_address,0,PERM_PRESENT );
					LIST_INSERT_HEAD(&(curenv->SecondList),temp_tail_active);
					pt_set_page_permissions(curenv->env_page_directory,temp_victim_head->virtual_address,PERM_PRESENT,0);
					LIST_INSERT_HEAD(&(curenv->ActiveList),temp_victim_head);
				}else{
					struct FrameInfo* frame;
					allocate_frame(&frame);
					map_frame(curenv->env_page_directory, frame, fault_va,PERM_PRESENT |PERM_USER | PERM_WRITEABLE );
					int ret= pf_read_env_page(curenv, (void *)fault_va);
					if (ret == E_PAGE_NOT_EXIST_IN_PF)
					{
						if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
								||(fault_va >= USTACKBOTTOM && fault_va < USTACKTOP))
						{
								  //do no thing
						}
						else
						{
							cprintf("faulted_va :%x\n", fault_va);
							sched_kill_env(curenv->env_id);
						}
					 }
					new_wse = NULL;
					struct WorkingSetElement* tail_active  = LIST_LAST(&(curenv->ActiveList));
					struct WorkingSetElement* temp_tail_active  =LIST_LAST(&(curenv->ActiveList));
					struct WorkingSetElement* tail_victim = LIST_LAST(&(curenv->SecondList));
					//struct WorkingSetElement* temp_tail_victim  = LIST_LAST(&(curenv->SecondList));

					uint32 page_permissions =
							pt_get_page_permissions(curenv->env_page_directory,tail_victim->virtual_address);
					if(page_permissions &  PERM_MODIFIED){
						uint32* ptr_page_table;
						struct FrameInfo* temp_victim_frame =
							get_frame_info(curenv->env_page_directory, tail_victim->virtual_address, &ptr_page_table);
						pf_update_env_page(curenv,tail_victim->virtual_address,temp_victim_frame);
					}
					unmap_frame(curenv->env_page_directory,tail_victim->virtual_address);

					LIST_REMOVE(&(curenv->SecondList),tail_victim);
					tail_victim->virtual_address = fault_va;
					LIST_REMOVE(&(curenv->ActiveList),tail_active);
					pt_set_page_permissions(curenv->env_page_directory,temp_tail_active->virtual_address,0,PERM_PRESENT);
					LIST_INSERT_HEAD(&(curenv->SecondList),temp_tail_active);
					LIST_INSERT_HEAD(&(curenv->ActiveList),tail_victim);


				}
				//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");

				//env_page_ws_print(curenv);

				 //TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
			}
		}




}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



