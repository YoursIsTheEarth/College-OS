/*
 File: kernel.C
 
 Author: R. Bettati
 Department of Computer Science
 Texas A&M University
 Date  : 12/09/03
 
 
 This file has the main entry point to the operating system.
 
 */


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"
#include "gdt.H"
#include "idt.H"          /* LOW-LEVEL EXCEPTION MGMT. */
#include "irq.H"
#include "exceptions.H"
#include "interrupts.H"

#include "simple_keyboard.H" /* SIMPLE KB DRIVER */
#include "simple_timer.H" /* TIMER MANAGEMENT */

#include "page_table.H"
#include "paging_low.H"

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
#define KERNEL_POOL_START_FRAME ((2 MB) / Machine::PAGE_SIZE)
#define KERNEL_POOL_SIZE ((2 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_START_FRAME ((4 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_SIZE ((28 MB) / Machine::PAGE_SIZE)
/* definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / Machine::PAGE_SIZE)
#define MEM_HOLE_SIZE ((1 MB) / Machine::PAGE_SIZE)
/* we have a 1 MB hole in physical memory starting at address 15 MB */

#define FAULT_ADDR (4 MB)
/* used in the code later as address referenced to cause page faults. */
#define NACCESS ((1 MB) / 4)
/* NACCESS integer access (i.e. 4 bytes in each access) are made starting at address FAULT_ADDR */

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

    /* -- GENERATE MEMORY REFERENCES */
    
    int *foo = (int *) FAULT_ADDR;
    int i;

    for (i=0; i<NACCESS; i++) {
	/*
	debug_out_E9_msg_value("i", i);
	debug_out_E9_msg_value("address of foo[i]", (unsigned int) &i);
	*/
        foo[i] = i;
    }

    PageTable::print_present_entries_to_file();
    
    Console::puts("DONE WRITING TO MEMORY. Now testing...\n");

    for (i=0; i<NACCESS; i++) {
        if(foo[i] != i) {
            Console::puts("TEST FAILED for access number:");
            Console::putui(i);
            Console::puts("\n");
            break;
        }
    }
    if(i == NACCESS) {
        Console::puts("TEST PASSED\n");
    }

int main() {
    
    GDT::init();
    Console::init();
    IDT::init();
    ExceptionHandler::init_dispatcher();
    IRQ::init();
    InterruptHandler::init_dispatcher();
    
    
    /* -- EXAMPLE OF AN EXCEPTION HANDLER -- */
    
    class DBZ_Handler : public ExceptionHandler {
      /* We derive Division-by-Zero handler from ExceptionHandler 
         and overoad the method handle_exception. */
    public:
        virtual void handle_exception(REGS * _regs) {
            Console::puts("DIVISION BY ZERO!\n");
            for(;;);
        }
    } dbz_handler;
    
    /* Register the DBZ handler for exception no.0 
       with the exception dispatcher. */
    ExceptionHandler::register_handler(0, &dbz_handler);
    

    /* -- INITIALIZE THE TIMER (we use a very simple timer).-- */
    
    SimpleTimer timer(100); /* timer ticks every 10ms. */
    
    /* ---- Register timer handler for interrupt no.0 
            with the interrupt dispatcher. */
    InterruptHandler::register_handler(0, &timer);
    
    /* NOTE: The timer chip starts periodically firing as
     soon as we enable interrupts.
     It is important to install a timer handler, as we
     would get a lot of uncaptured interrupts otherwise. */
    
    /* -- INSTALL KEYBOARD HANDLER -- */
    SimpleKeyboard::init();

    Console::puts("after installing keyboard handler\n");

    /* -- ENABLE INTERRUPTS -- */
    
    Machine::enable_interrupts();

    /* Dilma - for class */
    unsigned int var = 10;
    Console::putsui("var is: ", var);
    Console::putsui("\nphysical address of var is: ", (unsigned int) &var);
    Console::putsui(" (this is page frame ",  (unsigned int) & var / 4096 );
    Console::putsui(" and offset ",  (unsigned int)  var % 4096 );

    Console::puts("Now output to file");
    debug_out_E9((char*)"test");
    debug_out_E9((char*)"another test");
    outportb(0xe9, 'H');
    outportb(0xe9, 'I');
    Console::puts("\nIt will loop now\n");
    
    for(;;);

    /* -- INITIALIZE FRAME POOLS -- */

    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0,
                                  0);

    kernel_mem_pool.print_bitmap();

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    
    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    Console::puts("pmpif = "); Console::puti(process_mem_pool_info_frame); Console::puts("\n");
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame,
				   n_info_frames);
    
    /* Take care of the hole in the memory. */
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);
    
    /* -- INITIALIZE MEMORY (PAGING) -- */
    
    /* ---- INSTALL PAGE FAULT HANDLER -- */
    
    class PageFault_Handler : public ExceptionHandler {
        /* We derive the page fault handler from ExceptionHandler 
           and overload the method handle_exception. */
    public:
        virtual void handle_exception(REGS * _regs) {
            PageTable::handle_fault(_regs);
        }
    } pagefault_handler;
    
    /* ---- Register the page fault handler for exception no.14 
            with the exception dispatcher. */
    ExceptionHandler::register_handler(14, &pagefault_handler);
    
    /* ---- INITIALIZE THE PAGE TABLE -- */
    
    PageTable::init_paging(&kernel_mem_pool,
                           &process_mem_pool,
                           4 MB);
    
    PageTable pt;
    
    pt.load();

    Console::puts("after pt.load");
    
    PageTable::enable_paging();
    
    Console::puts("after enable_paging");

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    /*
    Console::puts("Hello World!\n");
    Console::puts("For decimal 7 hexa should be 0x0007\n");
    Consoel::puts(uint2hexstr(7));
    Console::puts("For decimal 4096 hexa should be 0x1000\n");
    Consoel::puts(uint2hexstr(4096));
        Console::puts("For decimal 3098674 hexa should be 02F4832\n");
    Consoel::puts(uint2hexstr(3098674));
    
    for(;;);
    */

    P3_original_test();
    P3_larger_test();
    /* -- GENERATE MEMORY REFERENCES */
    
    int *foo = (int *) FAULT_ADDR;
    int i;

    for (i=0; i<NACCESS; i++) {
	/*
	debug_out_E9_msg_value("i", i);
	debug_out_E9_msg_value("address of foo[i]", (unsigned int) &i);
	*/
        foo[i] = i;
    }

    PageTable::print_present_entries_to_file();
    
    Console::puts("DONE WRITING TO MEMORY. Now testing...\n");

    for (i=0; i<NACCESS; i++) {
        if(foo[i] != i) {
            Console::puts("TEST FAILED for access number:");
            Console::putui(i);
            Console::puts("\n");
            break;
        }
    }
    if(i == NACCESS) {
        Console::puts("TEST PASSED\n");
    }

    /* -- STOP HERE */
    Console::puts("YOU CAN SAFELY TURN OFF THE MACHINE NOW.\n");
    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}
