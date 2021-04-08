/*
 File: scheduler.C
 
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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Node::Node() {
	thread = NULL;
	next = NULL;
}

Node::Node(Thread *new_thread) {
	thread = new_thread;
	next = NULL;
}

void Node::push (Thread *new_thread) {
	if (thread == NULL) {
		thread = new_thread;
	}
	else {
		if (next != NULL) {
			next->push(new_thread);
		}
		else {
			next = new Node(new_thread);
		}
	}
}

Thread* Node::pop () {
	if (next != NULL) {
		Thread* ret_val = thread;
		Node* del_node = next;
		
		thread = next->thread;
		next = next->next;
		
		delete del_node;
		return ret_val;
	}
	else {
		Thread* ret_val = thread;
		thread = NULL;
		return ret_val;
	}
}

Scheduler::Scheduler() {
	size = 0;
	Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
	if (size > 0){
		Thread* current = queue.pop();
		Thread::dispatch_to(current);
		size--;
    }
}

void Scheduler::resume(Thread * _thread) {
	queue.push(_thread);
	size++;
}

void Scheduler::add(Thread * _thread) {
	queue.push(_thread);
	size++;
}

void Scheduler::terminate(Thread * _thread) {
	for (int i = 0; i < size; i++) {
		Thread* current = queue.pop();
		if (_thread->ThreadId() == current->ThreadId()) {
			size--;
		}
		else {
			queue.push(current);
		}
	}
}
