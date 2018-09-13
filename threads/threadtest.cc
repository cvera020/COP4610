// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

#define HW1_SEMAPHORES

int SharedVariable;

#ifdef HW1_SEMAPHORES
Semaphore* semaphore;
Semaphore* barrier;
int BarrierCounter;
#endif



void SimpleThread(int which)
{
    int num, val;
    
    for( num = 0; num < 5; num++ ) {
#ifdef HW1_SEMAPHORES
        semaphore->P(); //request the main semaphore
#endif
        val = SharedVariable;
        printf("*** thread %d sees value %d\n", which, val);
        currentThread->Yield();
        SharedVariable = val+1;
        
#ifdef HW1_SEMAPHORES
        semaphore->V(); //release the main semaphore
#endif
        currentThread->Yield();
    }

#ifdef HW1_SEMAPHORES
    barrier->P(); //request the barrier semaphore
    BarrierCounter--;   //indicate that one of the threads has finished processing
                        //and is now waiting on any others
    barrier->V(); //release the barrier semaphore
    while (BarrierCounter > 0) {
        currentThread->Yield();
    }
#endif    
    
    val = SharedVariable;
    printf("Thread %d sees final value %d\n", which, val);
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between n threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1(int n)
{    
    DEBUG('t', "Entering ThreadTest1");

    Thread *t[n];
    while (--n > 0) {
        t[n] = new Thread("forked thread");
        t[n]->Fork(SimpleThread, n);
    }
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest(int n)
{
    if (n == 0) {
        return;
    } else if (n < 0) {
        printf("No test specified.\n");
    }
    
#ifdef HW1_SEMAPHORES //instantiate and allocate semaphore variables and barrier counter
    BarrierCounter = n;
    semaphore = new Semaphore("s", 1);
    barrier = new Semaphore("b", 1);
#endif
    
    ThreadTest1(n);
    
#ifdef HW1_SEMAPHORES //deallocate semaphore variables
    delete semaphore;
    semaphore = 0;
    delete barrier;
    barrier = 0;
#endif
}

