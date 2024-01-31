/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
struct MemBlock_LIST mlist;
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{

	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized = 1;
	//=========================================
	//=========================================

	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//panic("initialize_dynamic_allocator is not implemented yet");
	LIST_INIT(&mlist);

	struct BlockMetaData *block=(struct BlockMetaData *)daStart; //kernel_heap

	block->is_free=1;

	block->size=initSizeOfAllocatedSpace; //3mega

	LIST_INSERT_HEAD(&mlist, block);

}



//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
uint32 total_size = DYN_ALLOC_MAX_SIZE;
bool direct_allocate = 1;
void *alloc_block_FF(uint32 size)
{

	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	//panic("alloc_block_FF is not implemented yet");
	if(size <= 0)
		return NULL;

	if(size > total_size)
		return NULL;


	if (!is_initialized)
	{
	uint32 required_size = size + sizeOfMetaData();
	uint32 da_start = (uint32)sbrk(required_size);
	//get new break since it's page aligned! thus, the size can be more than the required one
	uint32 da_break = (uint32)sbrk(0);
	initialize_dynamic_allocator(da_start, da_break - da_start);

	}
	struct BlockMetaData *blockMetaData = mlist.lh_first; //daStart
	struct BlockMetaData *blockEntryData;
	if(direct_allocate) {
		blockMetaData = mlist.lh_last;
	if(blockMetaData->is_free==1 &&(blockMetaData->size) >= size + sizeOfMetaData()){

			if(blockMetaData->size ==  size + sizeOfMetaData()) {

								blockMetaData->is_free= 0;
								blockMetaData->size = size + sizeOfMetaData();
								void* res = (void *) ((unsigned int )blockMetaData +(int)sizeOfMetaData());
								total_size-=(size + sizeOfMetaData());
								return res;
							}


							int newSize = blockMetaData->size - (size + sizeOfMetaData());

							blockEntryData = blockMetaData;

							blockEntryData->size = size + sizeOfMetaData();

							if(newSize > sizeOfMetaData()) {

								blockMetaData = (void *)((unsigned int)blockMetaData + (int)(size + sizeOfMetaData()));


								blockMetaData->size = newSize;

								blockMetaData->is_free = 1;


								LIST_INSERT_AFTER(&mlist,blockEntryData, blockMetaData);

							}
							else blockEntryData->size += newSize; //internal fragmentation

							blockEntryData->is_free = 0;

							void * res = (void*)((unsigned int )blockEntryData + (int)sizeOfMetaData());
							total_size-=(size + sizeOfMetaData());
							return res;

		}


  	blockEntryData = sbrk(size + sizeOfMetaData());

if(blockEntryData == (void *)-1)
	return NULL;
//UNSEEN UNTIL WE IMPLEMENT THE SBRK
  blockEntryData->is_free = 0;
  blockEntryData->size = size + sizeOfMetaData();
  LIST_INSERT_TAIL(&mlist, blockEntryData);
  int newSize = (uint32)sbrk(0) - ((uint32)blockEntryData + size + sizeOfMetaData());
	if(newSize > sizeOfMetaData()) {
		blockMetaData = (void *)((unsigned int)blockEntryData + (int)(size + sizeOfMetaData()));
		blockMetaData->size = newSize;
		blockMetaData->is_free = 1;
		LIST_INSERT_AFTER(&mlist,blockEntryData, blockMetaData);
	}
	else blockEntryData->size += newSize;
	void * res = (void*)((unsigned int )blockEntryData + (int)sizeOfMetaData());
	return res;

	}


	//BlockMetaData List of blocks
	direct_allocate = 1;
		LIST_FOREACH(blockMetaData, &mlist) {
//			cprintf("ELBLOCK GWA ELFOR = %p\n", blockMetaData);

			if(blockMetaData->is_free==1 &&(blockMetaData->size) >= size + sizeOfMetaData()) {
				direct_allocate = 0;
				if(blockMetaData->size ==  size + sizeOfMetaData()) {

					blockMetaData->is_free= 0;
					blockMetaData->size = size + sizeOfMetaData();
					void* res = (void *) ((unsigned int )blockMetaData +(int)sizeOfMetaData());
					total_size-=(size + sizeOfMetaData());
					return res;
				}


				int newSize = blockMetaData->size - (size + sizeOfMetaData());

				blockEntryData = blockMetaData;

				blockEntryData->size = size + sizeOfMetaData();

				if(newSize > sizeOfMetaData()) {

					blockMetaData = (void *)((unsigned int)blockMetaData + (int)(size + sizeOfMetaData()));


					blockMetaData->size = newSize;

					blockMetaData->is_free = 1;


					LIST_INSERT_AFTER(&mlist,blockEntryData, blockMetaData);

				}
				else blockEntryData->size += newSize; //internal fragmentation

				blockEntryData->is_free = 0;

				void * res = (void*)((unsigned int )blockEntryData + (int)sizeOfMetaData());
				total_size-=(size + sizeOfMetaData());
				return res;
			}
		}//forEach



      	blockEntryData = sbrk(size + sizeOfMetaData());

	if(blockEntryData == (void *)-1)
		return NULL;
	//UNSEEN UNTIL WE IMPLEMENT THE SBRK
	  blockEntryData->is_free = 0;
	  blockEntryData->size = size + sizeOfMetaData();
	  LIST_INSERT_TAIL(&mlist, blockEntryData);
	   int newSize = (uint32)sbrk(0) - ((uint32)blockEntryData + size + sizeOfMetaData());
		if(newSize > sizeOfMetaData()) {
			blockMetaData = (void *)((unsigned int)blockEntryData + (int)(size + sizeOfMetaData()));
			blockMetaData->size = newSize;
			blockMetaData->is_free = 1;
			LIST_INSERT_AFTER(&mlist,blockEntryData, blockMetaData);
		}
		else blockEntryData->size += newSize;
		blockEntryData->is_free = 0;
		void * res = (void*)((unsigned int )blockEntryData + (int)sizeOfMetaData());
		return res;

}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
//	panic("alloc_block_BF is not implemented yet");
	struct BlockMetaData *blockMetaData = mlist.lh_first; //daStart
		struct BlockMetaData *blockEntryData;
		int minfrag=-1;
		LIST_FOREACH(blockMetaData, &mlist) {
		if(blockMetaData->is_free==1 &&(blockMetaData->size)>= size + sizeOfMetaData()) {
			if(blockMetaData->size ==  size + sizeOfMetaData()) {
				blockMetaData->is_free= 0;
				blockMetaData->size = size + sizeOfMetaData();
				void* res = (void *) ((unsigned int )blockMetaData +(int)sizeOfMetaData());
				return res;
			}
			if(minfrag>(blockMetaData->size-(size + sizeOfMetaData()))||minfrag==-1)
	             minfrag=blockMetaData->size-(size + sizeOfMetaData());

					}
				}//forEach


				if(minfrag==-1){
					if(sbrk(size + sizeOfMetaData()) == (void *)-1)
						return NULL;

					//UNSEEN UNTIL WE IMPLEMENT THE SBRK
					if(blockMetaData->is_free == 1) {
						blockMetaData->is_free = 0;
						blockMetaData->size = size+sizeOfMetaData();
						return blockMetaData + 1;
						}

						blockEntryData = blockMetaData + blockMetaData->size;
						blockEntryData->is_free = 0;
						blockEntryData->size = size + sizeOfMetaData();
						LIST_INSERT_AFTER(&mlist,blockMetaData, blockEntryData);
						return blockEntryData + 1;
					}


				LIST_FOREACH(blockMetaData, &mlist) {
					if((blockMetaData->size-(size + sizeOfMetaData()))==minfrag&&blockMetaData->is_free==1)
					{

				int newSize = blockMetaData->size - (size + sizeOfMetaData());
				blockEntryData = blockMetaData;
				if(newSize > sizeOfMetaData()) {
					blockMetaData = (void *)((unsigned int)blockMetaData + (int)(size + sizeOfMetaData()));
					blockMetaData->size = newSize;
					blockMetaData->is_free = 1;
					LIST_INSERT_AFTER(&mlist,blockEntryData, blockMetaData);
				}
				if(newSize <= sizeOfMetaData())
					blockEntryData->size += newSize;
				blockEntryData->is_free = 0;
				blockEntryData->size = size + sizeOfMetaData();
				if(newSize <= sizeOfMetaData())
					blockEntryData->size += newSize;

				void * res = (void*)((unsigned int )blockEntryData + (int)sizeOfMetaData());
				return res;

					}
				}

		return NULL;

}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");
	direct_allocate = 0;
	struct BlockMetaData *blocktodel=((struct BlockMetaData *)(va -(void*)sizeOfMetaData()));
	if (blocktodel==NULL||blocktodel->is_free==1)
		return;

	struct BlockMetaData *blockprev = LIST_PREV(blocktodel);
	struct BlockMetaData *blockafter = LIST_NEXT(blocktodel);
	if (blockprev==NULL)//first element
	{
		if (blockafter->is_free==1)
		{
			blocktodel->size+=blockafter->size;
			LIST_REMOVE(&mlist,blockafter);
			blockafter->is_free=0;
			blockafter->size=0;

		}
		blocktodel->is_free=1;
	}
	else if (blockafter==NULL)//last element
	{
		if (blockprev->is_free==1)
		{
			blockprev->size+=blocktodel->size;
			LIST_REMOVE(&mlist,blocktodel);
			blocktodel->is_free=0;
			blocktodel->size=0;

		}
		else
		{
			blocktodel->is_free=1;
		}
	}
	else
	{
		blocktodel->is_free=1;
		if (blockafter->is_free==1)
		{
			blocktodel->size+=blockafter->size;
			LIST_REMOVE(&mlist,blockafter);
			blockafter->size=0;
			blockafter->is_free=0;

		}
		if (blockprev->is_free==1)
		{
			blockprev->size+=blocktodel->size;
			LIST_REMOVE(&mlist,blocktodel);
			blocktodel->is_free=0;
			blocktodel->size=0;

		}
	}


}


//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");
	struct BlockMetaData* needblock = ((struct BlockMetaData*)va - 1);
	if(new_size < needblock->size)
		direct_allocate = 0;
	if(va==NULL || needblock == NULL)
	{
		if(new_size==0) //va == null && new_size == 0
		{
			return NULL;
		}
		return alloc_block_FF(new_size); //va == null && new_size = new_size

	}
		if(new_size==0) //va == va && new_size = 0
		{

			free_block(va);
			return NULL;

		}

		if(needblock->size == new_size + sizeOfMetaData()) //va == va && new_size == old_size (Do Nothing)
			return va;

		struct BlockMetaData* nextBlock = LIST_NEXT(needblock);
		if (needblock->size < new_size + sizeOfMetaData()) { //increase
					//next is free and no reallocate
			 	 if(nextBlock != NULL && nextBlock->is_free == 1 && (needblock->size + nextBlock->size) >= (new_size + sizeOfMetaData())) {
			 		 //no split
			 		 if((needblock->size + nextBlock->size) == (new_size + sizeOfMetaData())){
			 			 needblock->size += nextBlock->size;
			 			 LIST_REMOVE(&mlist, nextBlock);
			 			 nextBlock->is_free = 0;
			 			 nextBlock->size = 0;
			 			 return needblock + 1;
			   }
			   //split
			   uint32 free_size = (needblock->size + nextBlock->size) - (new_size + sizeOfMetaData());
			   needblock->size = new_size + sizeOfMetaData();
			   LIST_REMOVE(&mlist, nextBlock);
			   nextBlock->is_free = 0;
			   nextBlock->size = 0;
			   //if split > sizeOfMetaData then split else add it to the allocated block as an internal fragmentation
			   if(free_size > sizeOfMetaData()) {

				   nextBlock = (struct BlockMetaData*)((uint32)needblock + needblock->size); //new address
				   nextBlock->size = free_size;
				   nextBlock->is_free = 1;
				   LIST_INSERT_AFTER(&mlist, needblock, nextBlock);
			   }
			   else needblock->size += free_size; //internal fragmentation
			   return needblock + 1;
			 }
				//reallocate
			 	uint32 old_size = needblock->size;
				void* ret = alloc_block_FF(new_size); //new allocated block
				if(ret == NULL) return va; //if NULL even with the sbrk() then Do Nothing
				short* sva = va;
				for(short* i = (short*)ret; i < (short*)((uint32)ret + old_size); i++, sva++) {
					*(i) = *(sva);
				}
				free_block(va); //after allocating the block somewhere with the new size, free the old allocated block
				return ret; //return the new allocated block

		 }

		//Decrease
		 if (nextBlock != NULL && nextBlock->is_free == 1) { //if the next is free (care of vanishing!!)
			  uint32 free_size = (needblock->size + nextBlock->size) - (new_size + sizeOfMetaData());
			  needblock->size = new_size + sizeOfMetaData();
			  nextBlock->is_free = 0; //vanishing the old addressed block and create new free with new size
			  nextBlock->size = 0;
			  LIST_REMOVE(&mlist, nextBlock);
			  nextBlock = (struct BlockMetaData*)((uint32)needblock + needblock->size); //new address
			  nextBlock->size = free_size; //new size
			  nextBlock->is_free = 1;
			  return needblock + 1;
		 }
		 //create new freeblock between them
		 if(needblock->size - new_size - sizeOfMetaData() > sizeOfMetaData()) { //if the created free less than the metadata Do Nothing!
		 uint32 free_size = (needblock->size) - (new_size + sizeOfMetaData());
		 needblock->size = new_size + sizeOfMetaData();
		 struct BlockMetaData *newFree = (void *)((uint32)needblock + (int)needblock->size);
		 newFree->is_free = 1;
		 newFree->size = free_size;
		 LIST_INSERT_AFTER(&mlist, needblock, newFree);
		 }
		 return needblock+1;
}
