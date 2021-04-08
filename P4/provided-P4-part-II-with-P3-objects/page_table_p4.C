#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"
#include "vm_pool.H"

/*************************************************
 * The following methods are defined in the provided page_table_provided.o
 *
 * You can keep using your own implementation from P3, ignoring page_table_provided.o
 *


   void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size);

   PageTable::PageTable();

   void PageTable::load();

   void PageTable::enable_paging();

   void PageTable::handle_fault(REGS * _r);

  *************************************************
  */

VMPool* PageTable::VmPools[10];
int PageTable::PoolCount = 0;

bool PageTable::check_address(unsigned long address)
{
    return true;
	// you need to implement this
    // it returns true if legitimate, false otherwise
    for (int i = 0; i < PoolCount; i++) {
		if (VmPools[i]->is_legitimate(address)) {
			return true;
		}
	}
    return false; // you need to implement this
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    if (PoolCount < 10) {
		VmPools[PoolCount] = _vm_pool;
		PoolCount++;
		Console::puts("registered VM pool\n");
		return;
	}
	Console::puts("ERROR: VM pool array is full\n");
}

void PageTable::free_page(unsigned long _page_no) {
    unsigned long directory_index = _page_no >> 10;
	unsigned long table_index = _page_no & 0x3FF;
	
	ContFramePool::release_frames(_page_no);
	
	current_page_table->page_directory[directory_index] = 0x2;
	write_cr3(read_cr3());
    Console::puts("freed page\n");
}
