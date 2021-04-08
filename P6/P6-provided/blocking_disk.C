/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"

extern Scheduler * SYSTEM_SCHEDULER;


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
	issue_operation(READ, _block_no);

	blockedQueue.push(Thread::CurrentThread());
	
	while(!is_ready()) {
		if (blockedQueue.thread == Thread::CurrentThread()) {
			SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
			SYSTEM_SCHEDULER->yield();
		}
	}
	
	blockedQueue.pop();

	/* read data from port */
	int i;
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = Machine::inportw(0x1F0);
		_buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	}

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
	issue_operation(WRITE, _block_no);

	blockedQueue.push(Thread::CurrentThread());
	
	while(!is_ready()) {
		if (blockedQueue.thread == Thread::CurrentThread()) {
			SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
			SYSTEM_SCHEDULER->yield();
		}
	}
	
	blockedQueue.pop();

	/* write data to port */
	int i; 
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
		Machine::outportw(0x1F0, tmpw);
	}
}
