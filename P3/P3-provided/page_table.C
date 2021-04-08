#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool =  _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   
   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
	unsigned long frame = kernel_mem_pool->get_frames(1);
	page_directory = (unsigned long*)(frame * PAGE_SIZE);
	
	frame = kernel_mem_pool->get_frames(1);
	unsigned long *page_table = (unsigned long *)(frame * PAGE_SIZE);
	
	unsigned long address=0; // holds the physical address of where a page is

	// map the first 4MB of memory
	for(unsigned int i = 0; i < 1024; i++)
	{
		page_table[i] = address | 3;
		address = address + 4096;
	};
	
	page_directory[0] = (unsigned long) page_table;
	page_directory[0] = page_directory[0] | 3;
	
	for(unsigned int i = 1; i < 1024; i++)
	{
		page_directory[i] = 0 | 2;
	};
	
	Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{	
	current_page_table = this;
	write_cr3((unsigned long) page_directory);
	Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   write_cr0(read_cr0() | 0x80000000);
   paging_enabled = 1;
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  Machine::enable_interrupts();
  
  unsigned long address = read_cr2();
  unsigned long page_number = address >> 12;
  
  unsigned long index = address >> 22;
  
  // Updating Page Directory
  
  if( (current_page_table->page_directory[index] & 0x1) != 0x1) {
	  unsigned long * new_page_table = (unsigned long *)(kernel_mem_pool->get_frames(1) * PAGE_SIZE);
	  for (int i = 0; i < PAGE_SIZE; i++) {
		  new_page_table[i] = 0x2;
	  }
	  current_page_table->page_directory[index] = (unsigned long) new_page_table;
	  current_page_table->page_directory[index] |= 0x3;
  }
  
  unsigned long * page_table_page = (unsigned long *) (current_page_table->page_directory[index] & 0xFFFFF000);
  unsigned int page_index = page_number & (0x000003FF);
  
  unsigned long frame = process_mem_pool->get_frames(1);
  unsigned long frame_address = frame * PAGE_SIZE;
  page_table_page[page_index] = frame_address | 0x3;

  Console::puts("handled page fault\n");
}

