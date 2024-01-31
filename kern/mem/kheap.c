#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"



int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...  f009fff5 4027187189
//	panic("not implemented yet");
	hstart=daStart;
	brk=ROUNDUP(daStart+initSizeToAllocate,PAGE_SIZE);
	hardlimit=daLimit;
	if(brk <= hardlimit)
	{
		for(uint32 i=hstart;i<brk;i+=PAGE_SIZE)
		{
			struct FrameInfo *ptr=NULL;
			int ret=allocate_frame(&ptr);
			map_frame(ptr_page_directory,ptr,i, PERM_PRESENT|PERM_WRITEABLE);
			ptr->va = i;

		}
	  initialize_dynamic_allocator(hstart, initSizeToAllocate);
	   return 0;
	}

	else
		return E_NO_MEM;


}



void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 *
	 * 	Remember! start = h_start, sbrk = h_sbrk, hard limit = h_hard_limit
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING====
		//2.If increment = 0: just return the current position of the segment break
	if(increment == 0)
		return (void*)brk;


	int new_increment;
	//1.If increment > 0: if within the hard limit
	if(increment > 0) {
		uint32 old_sbrk = brk;
		brk = ROUNDUP(brk + increment, PAGE_SIZE);
		if(brk > hardlimit)
			panic("HARDLIMIT !!");

		//1.1.move the segment break of the kernel to increase the size of its heap

		//1.2.allocate pages and map them into the kernel virtual address space as necessary
		struct FrameInfo* ptr_frame_info;
		for(uint32 i =ROUNDUP(old_sbrk, PAGE_SIZE) ; i < brk; i += PAGE_SIZE) {
				allocate_frame(&ptr_frame_info);
				ptr_frame_info->va = i;
				map_frame(ptr_page_directory, ptr_frame_info, i, PERM_PRESENT | PERM_WRITEABLE);
		}

		//1.3.returns the address of the previous break (i.e. the beginning of newly mapped memory)
		return (void *)old_sbrk;
	}

	//3.If increment < 0:
//	new_increment = ROUNDDOWN(increment, PAGE_SIZE); 3'lt
	//3.2.deallocate pages that no longer contain part of the heap as necessary.

	uint32 new_brk = brk + increment;

	for(uint32 i = ROUNDUP(new_brk, PAGE_SIZE) ; i < brk; i +=  PAGE_SIZE) {
		unmap_frame(ptr_page_directory, i);
	}
	//3.1.move the segment break of the kernel to decrease the size of its heap,
	brk += increment;
	//3.3.returns the address of the new break (i.e. the end of the current heap space).
	return (void *)brk;

//	return (void*)-1 ;
//	panic("not implemented yet");
}


//#define pageSize  (KERNEL_HEAP_MAX - KERNEL_HEAP_START)/PAGE_SIZE
//struct va_pages
//{
//	uint32 va;
//	uint32 size;
//
//};
//struct va_pages array[pageSize+1];

void* kmalloc(unsigned int size)
{

	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	void* kernelStart =(void*) ((uint32)hardlimit+PAGE_SIZE);
	void* firstFrame=kernelStart;
	if (isKHeapPlacementStrategyFIRSTFIT()==1) {
		struct FrameInfo *frame=free_frame_list.lh_first;
		uint32 maximumSpace=((uint32)KERNEL_HEAP_MAX-(uint32)kernelStart);
		uint32 numOfFrames = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;

		if(size<=DYN_ALLOC_MAX_BLOCK_SIZE) {

			return alloc_block_FF(size);
		}else{
			size = ROUNDUP(size, PAGE_SIZE);
			int availableSize =0;
			uint32* ptrtopagetable=NULL;
			while(firstFrame!=(void*)KERNEL_HEAP_MAX){
				uint32 free = get_frame_info(ptr_page_directory,(uint32)firstFrame, &ptrtopagetable)==0;
				if(availableSize>=size) {
					firstFrame -= availableSize;
					break;
				}
				if(free==1) availableSize+=PAGE_SIZE;
				else availableSize=0;

				firstFrame+=PAGE_SIZE;
			}

				if(size>availableSize)return NULL;
				availableSize = size;
				while(availableSize>0){
					int ret = allocate_frame(&frame);
				//	frame->references++;
					frame->va = (uint32)firstFrame;
					frame->size = availableSize;
					if(ret!=E_NO_MEM){
						map_frame(ptr_page_directory,frame,(uint32)firstFrame,PERM_PRESENT |PERM_WRITEABLE);
						firstFrame+=PAGE_SIZE;
					}else{
						return NULL;
					}

					//count_frames++;
					availableSize-=PAGE_SIZE;
				}
				}
					uint32 result = (uint32)firstFrame-(numOfFrames*PAGE_SIZE);
//					int i = (result - (hardlimit + PAGE_SIZE))/PAGE_SIZE;
//					array[i].va=(uint32)firstFrame-(numOfFrames*PAGE_SIZE);
//					array[i].size=ROUNDUP(size,PAGE_SIZE);
					//cprintf("size no %u %u %u\n",size,numOfFrames,framesAalloc);


					return (void*) (result);
		}


	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");
	if (isKHeapPlacementStrategyFIRSTFIT()==1){
	if (virtual_address>=(void *)hstart&&virtual_address<(void *)hardlimit)
	{
		free_block(virtual_address);
	}
	else if (virtual_address>=(void *)(hardlimit+PAGE_SIZE)&&virtual_address<(void *)KERNEL_HEAP_MAX)
	{
//		int i=0;
//		uint32 va=array[i].va;
//		while (va!=(uint32)virtual_address)
//		{
//			i++;
//			va=array[i].va;
//		}
//		uint32 size =array[i].size;
		//cprintf("frames to free %d \n",size/PAGE_SIZE);
		uint32* ptr_page_table;
		struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);

		if (ptr_frame_info)
		{
			uint32 va = (uint32) virtual_address;
			int size = ptr_frame_info->size;
			while(size){
				unmap_frame(ptr_page_directory, va);
				size-=PAGE_SIZE;
				va+=PAGE_SIZE;
			}
//			uint32 * ptrtopagetable=NULL;
//			struct FrameInfo * frametofree;
//			frametofree=get_frame_info(ptr_page_directory,(uint32)va, &ptrtopagetable);
			//free_frame(frametofree);
//			va+=PAGE_SIZE;
//			size-=PAGE_SIZE;
			//count_free++;
		}
//		i = ((uint32)virtual_address - (hardlimit + PAGE_SIZE))/PAGE_SIZE;
//		array[i].va=(uint32)NULL;
//		array[i].size=0;


	}
	else
	{
		panic("Invalid address...!!")	;
	}
	}
}

unsigned int kheap_virtual_address(unsigned int physical_address) {
//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address()

        struct FrameInfo *i=to_frame_info(physical_address);
        unsigned int offset=physical_address%PAGE_SIZE;
        if (i!=NULL && i->va>=KERNEL_HEAP_START )
        {
        	return (i->va + offset);
        }
        return 0;
}
unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
    uint32 pt_index=PTX(virtual_address);//second 10 pits in va(page table index)
	//f# of page(physical address)
	uint32 *ptr_page_table=NULL;
	get_page_table(ptr_page_directory,virtual_address,&ptr_page_table);
	if(ptr_page_table!=NULL){
	uint32 PTentry=ptr_page_table[pt_index];

	uint32 physical_address=(PTentry & 0xFFFFF000) + (virtual_address & 0x00000FFF);
	return physical_address;
	}
    //change this "return" according to your answer
	return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	if (isKHeapPlacementStrategyFIRSTFIT()==1) {
	uint32 va = (uint32) virtual_address;
	if(va == 0) {
		if(new_size == 0) //va = 0, new_size = 0 -> NULL
			return NULL;
		return kmalloc(new_size); //va = 0, new_size != 0 -> allocate
	}

	if(va >= hstart && va < hardlimit) { //in block range
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE) //va != 0, size in block range(even 0)
			return realloc_block_FF((void*)va, new_size); //from block to block

		//from block to page
		struct BlockMetaData* block = virtual_address;
		uint32 old_size = block->size;
		void* ret = kmalloc(new_size); //from block to page
		if(ret == NULL) return virtual_address;
		char* sva = virtual_address; //save the data of the old page
		for(char* i = (char*)ret; i < (char*)((uint32)ret + old_size); i++, sva++) {
			*(i) = *(sva);
		}
		kfree((void*)va);
		return ret;

	}

	if(va >= hardlimit + PAGE_SIZE && va < KERNEL_HEAP_MAX) { // in page range
		if(new_size == 0) {  //new_size = 0 -> free
			kfree((void*)va);
			return NULL;
		}
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE) { //from page to block
			void* ret = kmalloc(new_size);
			if(ret == NULL) return virtual_address;
			char* sva = virtual_address; //save the data of the old page
			for(char* i = (char*)ret; i < (char*)((uint32)ret + new_size); i++, sva++) {
				*(i) = *(sva);
			}
			kfree((void*)va);
			return ret;
		}

		//from page to page
		new_size = ROUNDUP(new_size, PAGE_SIZE);
		uint32* ptr_page_table;
		struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, va, &ptr_page_table);
		if(ptr_frame_info == NULL || ptr_frame_info->va == 0) return NULL ; //ERROR
		//EQUAL DO NOTHING
		if(ROUNDUP(ptr_frame_info->size, PAGE_SIZE) == ROUNDUP(new_size, PAGE_SIZE)) {

			return (void*)va;
		}

		//INCREASE
		if(ROUNDUP(ptr_frame_info->size, PAGE_SIZE) < ROUNDUP(new_size, PAGE_SIZE)) {
			uint32 newVa = ROUNDUP(va + ptr_frame_info->size, PAGE_SIZE);
			uint32 endVa = ROUNDUP(va + new_size, PAGE_SIZE);
			int sizeFree = 0;
			for(uint32 i = newVa;  i < endVa; i += PAGE_SIZE) {
				struct FrameInfo* new_frame = get_frame_info(ptr_page_directory, i, &ptr_page_table);
				if(new_frame) break;
				sizeFree+=PAGE_SIZE;
			}
			struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, va, &ptr_page_table);
			//reallocate in different place
			if(sizeFree != ROUNDUP(new_size, PAGE_SIZE) - ptr_frame_info->size) {
				void* ret = kmalloc(new_size);
				if(ret == NULL) return virtual_address;
				uint32 old_size = ptr_frame_info->size;
				char* sva = virtual_address; //save the data of the old page
				for(char* i = (char*)ret; i < (char*)((uint32)ret + old_size); i++, sva++) {
					*(i) = *(sva);
				}
				kfree((void*)va);
				return ret;
			}
			//reallocate in same place
			uint32 rem = endVa - newVa;
			for(uint32 i = newVa; i < endVa ; i += PAGE_SIZE) {
				allocate_frame(&ptr_frame_info);
				map_frame(ptr_page_directory, ptr_frame_info, i, PERM_PRESENT | PERM_WRITEABLE);
				ptr_frame_info->size = rem;
				rem -= PAGE_SIZE;
			}
			ptr_frame_info = get_frame_info(ptr_page_directory, va, &ptr_page_table);
			ptr_frame_info->size = new_size;
			return (void*)va;
		}
		//DECREASE
		uint32 newVa = ROUNDUP(va + new_size, PAGE_SIZE);
		uint32 endVa = ROUNDUP(va + ptr_frame_info->size, PAGE_SIZE);
		uint32 newFree = ROUNDUP(ptr_frame_info->size, PAGE_SIZE) - ROUNDUP(new_size, PAGE_SIZE);
		for(uint32 i = newVa; i < endVa ; i+=PAGE_SIZE){
			unmap_frame(ptr_page_directory, i);
		}

		ptr_frame_info = get_frame_info(ptr_page_directory, va, &ptr_page_table);
		ptr_frame_info->size = new_size;
		return (void*)va;

		}
	}
	return NULL;
	//panic("krealloc() is not implemented yet...!!");

}


