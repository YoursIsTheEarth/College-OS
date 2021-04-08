/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
	
	head = (Region *) base_address;
	head->address = base_address;
	head->size = page_table->PAGE_SIZE;	
	tail = head;
	RegionsSize = page_table->PAGE_SIZE;
	RegionsCount = 1;
	
	page_table->register_pool(this);	
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    // USED ONLY IN P4 PART III
	if (_size + RegionsSize > this->size || RegionsCount == MaxRegions) {
		return 0;
	}
	
	unsigned long new_address;
	if (tail == NULL) {
		new_address = this->base_address;
		head = (Region*)new_address;
		head->address = new_address;
		head->size = _size;
		tail = head;
	}
	else {
		new_address = tail->address + tail->size;
		Region* temp = tail;
		tail = (Region*)(tail + sizeof(Region*));
		temp->next = tail;
		tail->address = new_address;
		tail->size = _size;
	}		
	
	RegionsSize += _size;
	RegionsCount++;
    Console::puts("Allocated region of memory.\n");
	return new_address;
}

void VMPool::release(unsigned long _start_address) {
    // USED ONLY IN P4 PART III
    if (head == NULL) {
		return;
	}
	Region* curr_region = head;
	while (curr_region->address != _start_address) {
		if (curr_region == tail) {
			return;
		}
		curr_region = curr_region->next;
	}
	
	unsigned long curr_addr = _start_address;
	unsigned long end =  curr_region->size + _start_address;
	while (curr_addr < end) {
		page_table->free_page(curr_addr);
    	curr_addr += PageTable::PAGE_SIZE;
	}
	
	RegionsSize -= curr_region->size;
	RegionsCount--;
	
	// Saving space
	// Empty List
	if (head == curr_region && curr_region == tail) {
		head = NULL;
		tail = NULL;
	}
	// Moving tail to replace released Region
	else {
		Region* new_tail = head;
		while (new_tail->next != tail) {
			new_tail = new_tail->next;
		}
		curr_region->address = tail->address;
		curr_region->size = tail->size;
		tail = new_tail;
	}
	
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    // IMPLEMENTATION FOR P4
	if (head == NULL) {
		return false;
	}
	
    Region* curr_region = head;
	while (true) {
		if (curr_region->address <= _address && _address < curr_region->address + curr_region->size) {
			return true;
		}
		if (curr_region == tail) {
			return false;
		}
		curr_region = curr_region->next;
	}
}

