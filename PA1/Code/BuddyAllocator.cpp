#include "BuddyAllocator.h"
#include <iostream>
#include <cmath>
using namespace std;

// Linked List Definition
// adds a block to the list
void LinkedList::insert (BlockHeader* b) {	
  b->next = head;
	head = b;
}
// removes a block from the list
void LinkedList::remove (BlockHeader* b) {  
  BlockHeader* curr = head;
  if (b == head) { // if the removed block is head
      head = curr->next;
      return;
  }
  while (curr) { // while curr is not NULL
    if (curr->next == b && curr->next->next) {
      curr->next = curr->next->next; // skip block b
    }
    curr = curr->next; // move curr to its next pointer
  }
}

// pops the head of the list
BlockHeader* LinkedList::pop() {
  BlockHeader* curr = head;
  head = head->next;
  return curr;  
}

// Buddy Allocator Definition
// Constructor
BuddyAllocator::BuddyAllocator (int _basic_block_size, int _total_memory_length) {
  basic_block_size = _basic_block_size, total_memory_size = _total_memory_length;
  // calculate the number of blocks by dividing total memory size to basic block size
  int num = num_of_blocks(total_memory_size);
  // initialize the vector by pushing back free linked lists
  for (int i = 0; i <= num; i++) {
    FreeList.push_back(LinkedList());
  }
  // allocates an array of char's of total_memory_size 
  base = new char[total_memory_size];
  // initialize the block header by its public parameter
	BlockHeader* base_head = (BlockHeader*) base;
  base_head->block_size = total_memory_size;
  base_head->next = NULL;
	base_head->allocated = false;
  // insert the base block header to the end of the vector
  FreeList[num].insert(base_head);
}

// Deallocator
BuddyAllocator::~BuddyAllocator () {
  // delete base and set it to NULL to avoid dangling pointer
  delete base;
  base = NULL;
}

void* BuddyAllocator::alloc(int _length) {
	/* Allocate _length number of bytes of free memory and returns the 
		address of the allocated portion. Returns 0 when out of memory. */ 
    
  // the actual length is the addition of allocated memory length and blockheader memory length
  int actual_length = _length + sizeof(BlockHeader);
  // calculate the number of blocks
  int num = num_of_blocks(actual_length);
  // initialize a block header
  BlockHeader* block;
  // check if the total memory size is big enough for allocation
  if (total_memory_size < actual_length) return NULL;
  if (!FreeList[num].head) { // if the head of the linked list at k is NULL
    // go to next index in the freelist
    int i = num + 1; 
    while (i > num) {
      if (FreeList[i].head) { // if the head of the linked list at i is not NULL
        // pop the head of the linked list in the last memory block
        block = FreeList[i].pop();
        // split the block and set it as newBlock
        BlockHeader* newBlock = split(block);
        i--;
        // insert the newBlock behind the original block
        FreeList[i].insert(newBlock);
        FreeList[i].insert(block);
      } else {
        i++; // check one by one until reaching the end of the freelist
        if (i >= FreeList.size()) return NULL;
      }
    }   
  }
  // pop the head of the freelist and set the block as allocated
  block = FreeList[num].pop();
  block->allocated = true;
  // return the void pointer pointning to the next address of block
  return (void*)(block + 1);
}

void BuddyAllocator::free(void* _a) {
  /* Same here! */
  BlockHeader* this_block = (BlockHeader*)((char*)_a - sizeof(BlockHeader));
  this_block->allocated = false;
  BlockHeader* buddy_block = getbuddy(this_block);
  int num = num_of_blocks(this_block->block_size);
  while (!buddy_block->allocated) { // while the buddy block is free
    // remove the block and its buddy block from the freelist
    FreeList[num].remove(this_block);
    FreeList[num].remove(buddy_block);
    // merge the two blocks to this_block
    this_block = merge(this_block, buddy_block);
    num++;
    // insert the block to the end of the freelist
    FreeList[num].insert(this_block);
    // check if the free list is empty
    if (this_block->block_size != total_memory_size) {
      buddy_block = getbuddy(this_block);
    } else { break; } // otherwise break the while loop
  }
}

BlockHeader* BuddyAllocator::getbuddy(BlockHeader * addr) {
// given a block address, this function returns the address of its buddy 
  int offset = (int)((char*)addr - base);
  int buddy_offset = offset^addr->block_size;
  char* buddy = base + buddy_offset;
  return (BlockHeader*)(buddy); // offset of the buddy
}

bool BuddyAllocator::arebuddies (BlockHeader* block1, BlockHeader* block2) {
// checks whether the two blocks are buddies are not
// note that two adjacent blocks are not buddies when they are different sizes
  return (getbuddy(block1)==block2) && (getbuddy(block2)==block1);
}

BlockHeader* BuddyAllocator::merge (BlockHeader* block1, BlockHeader* block2) {
// this function merges the two blocks returns the beginning address of the merged block
// note that either block1 can be to the left of block2, or the other way around
  if((BlockHeader*)((char*)block1 + block1->block_size) == block2) {
      block1->block_size *= 2;
      block2 = NULL;
      return block1;
  } else {
      block2->block_size *= 2;
      block1 = NULL;
      return block2;
  }
}

BlockHeader* BuddyAllocator::split (BlockHeader* block) {
// splits the given block by putting a new header halfway through the block
// also, the original header needs to be corrected
  BlockHeader* newBlock = (BlockHeader*)((char*)block + block->block_size/2);
  block->block_size /= 2;
  newBlock->block_size = block->block_size;
  newBlock->allocated = false;
  block->next = NULL;
  newBlock->next = NULL;
  return newBlock;
}

int BuddyAllocator::num_of_blocks(int total_size) {
  // calculate the number of blocks by dividing total memory size to basic block size
  // log2(total_size / basic_block_size) = log2(total_size)- log2(basic_block_size)
  int i = floor(ceil(log2(total_size)- log2(basic_block_size)));
  if (i < 0) { return 0; } 
  else { return i; }
}

void BuddyAllocator::printlist () {
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  int64_t total_free_memory = 0;
  for (int i = 0; i < FreeList.size(); i++) {
    int blocksize = ((1<<i) * basic_block_size); // all blocks at this level are this size
    cout << "[" << i <<"] (" << blocksize << ") : ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList[i].head;
    // go through the list from head to tail and count
    while (b) {
      total_free_memory += blocksize;
      count ++;
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->block_size != blocksize) {
        cerr << "ERROR:: Block is in a wrong list" << endl;
        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;
    cout << "Amount of available free memory: " << total_free_memory << " bytes" << endl;  
  }
}

