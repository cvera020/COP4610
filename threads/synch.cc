// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Lock
// 	Instantiates a lock object that uses an internal semaphore object
//----------------------------------------------------------------------
Lock::Lock(char* debugName) {
    name = debugName;
    semaphore = new Semaphore("SemaphoreForLock", 1);
}
//----------------------------------------------------------------------
// Lock::~Lock
// 	Deallocate the internal semaphore object
//----------------------------------------------------------------------
Lock::~Lock() {
    delete semaphore;
}
//----------------------------------------------------------------------
// Lock::Acquire
// 	Attempt to acquire the lock or block if it is currently being held
//----------------------------------------------------------------------
void Lock::Acquire() {
    semaphore->P();
}
//----------------------------------------------------------------------
// Lock::Release
// 	Release the lock if it is being held
//----------------------------------------------------------------------
void Lock::Release() {
    semaphore->V();
}


//----------------------------------------------------------------------
// Condition::Condition
// 	Constructor for a Condition object that uses an internal Lock object
//----------------------------------------------------------------------
Condition::Condition(char* debugName) {
    name = debugName;
    queue = new List;
}
//----------------------------------------------------------------------
// Condition::~Condition
// 	Destructor for a condition object
//----------------------------------------------------------------------
Condition::~Condition() {
}
//----------------------------------------------------------------------
// Condition::Wait
// 	Wait for the Condition to become free and acquire the condition
//      lock for currentThread
//----------------------------------------------------------------------
void Condition::Wait(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff); //turn off interrupts
    conditionLock->Release();
    queue->Append((void *) currentThread);
    currentThread->Sleep();
    conditionLock->Acquire();
    (void) interrupt->SetLevel(oldLevel); //revert to previous interrupt mode
}


//----------------------------------------------------------------------
// Condition::Signal
// 	Wake up one of the threads that is waiting on the Condition object
//----------------------------------------------------------------------
void Condition::Signal(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff); //turn off interrupts
    Thread *thrd = (Thread *)queue->Remove();
    if( thrd != NULL)
	scheduler->ReadyToRun(thrd);
    (void) interrupt->SetLevel(oldLevel); //revert to previous interrupt mode
}

//----------------------------------------------------------------------
// Condition::Signal
// 	Wake up all of the threads that are waiting on the Condition object
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    conditionLock->Release();
    Thread *thrd = (Thread *)queue->Remove();
    while(thrd != NULL)
    {
	scheduler->ReadyToRun(thrd);
	thrd = (Thread *)queue->Remove();  //iteratively remove all threads from the
                                        //Condition obj's queue
    }
    (void) interrupt->SetLevel(oldLevel);
}
