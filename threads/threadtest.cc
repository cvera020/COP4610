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

//#define HW1_SEMAPHORES
//#define HW1_LOCKS
//#define HW1_ELEVATOR - MUST BE DEFINED IN threads/main.cc for elevator to run

int SharedVariable;

#ifdef HW1_SEMAPHORES
Semaphore* semaphore;
Semaphore* barrier;
int BarrierCounter;
#endif
#ifdef HW1_LOCKS
Lock* lock;
Lock* lockBarrier;
int LockBarrierCounter;
#endif



void SimpleThread(int which)
{
    int num, val;
    
    for( num = 0; num < 5; num++ ) {
#ifdef HW1_SEMAPHORES
        semaphore->P(); //request the main semaphore
#endif
#ifdef HW1_LOCKS
        lock->Acquire();
#endif
        val = SharedVariable;
        printf("*** thread %d sees value %d\n", which, val);
        currentThread->Yield();
        SharedVariable = val+1;
        
#ifdef HW1_SEMAPHORES
        semaphore->V(); //release the main semaphore
#endif
#ifdef HW1_LOCKS
        lock->Release();
#endif
        currentThread->Yield();
    }

#ifdef HW1_SEMAPHORES
    barrier->P(); //request the barrier semaphore
    BarrierCounter--;   //indicates that one of the threads has finished processing
                        //and is now waiting on any others
    barrier->V(); //release the barrier semaphore
    while (BarrierCounter > 0) {
        currentThread->Yield();
    }
#endif

#ifdef HW1_LOCKS
    lockBarrier->Acquire();
    LockBarrierCounter--;   //indicates that one of the threads has finished processing
                            //and is now waiting on any others
    lockBarrier->Release();
    while (LockBarrierCounter > 0) {
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
    DEBUG('H', "\tUsing semaphores\n");
    BarrierCounter = n;
    semaphore = new Semaphore("s", 1);
    barrier = new Semaphore("b", 1);
#endif
    
#ifdef HW1_LOCKS //instantiate + allocate lock variables and barrier counter
    DEBUG('H', "\tUsing locks\n");
    LockBarrierCounter = n;
    lock = new Lock("l");
    lockBarrier = new Lock("b");
#endif
    
    ThreadTest1(n);
    
#ifdef HW1_SEMAPHORES //deallocate semaphore variables
    delete semaphore;
    semaphore = 0;
    delete barrier;
    barrier = 0;
#endif
#ifdef HW1_LOCKS
    delete lock;
    lock = 0;
    delete lockBarrier;
    lockBarrier = 0;
#endif
}



//#ifdef HW1_ELEVATOR
#define ELEVATOR_CAPACITY 5
#define TICKS_TO_NEXT_FLOOR 50
#define NO_TARGETTED_FLOOR 0

void Elevator(int numFloors);
void NewPersonThread(int numFloors);
void ArrivingGoingFromTo(int atFloor, int toFloor);
void PerformElevatorAction(int atToArgs);

struct PersonThread {
    int id;
    int atFloor;
    int toFloor;
};

struct ElevatorThread {
    int numFloors;
    int currentFloor;
    int numPeopleIn;
    int targettedFloors[ELEVATOR_CAPACITY]; //circular array to keep track of  
                                            //which floors the elevator will
                                            //stop at
    int targettedFloorIndex; //index of which floor will receive priority for
                             //the elevator to move towards
};


Semaphore* insideElevatorSemaphore;
Semaphore* waitForElevatorSemaphore;

int idCounter; //shared variable to produce unique ids
Lock* personCreateLock; //used to limit access to idCounter
Lock* elevatorAccessLock; //used to limit access to elevator struct fields

struct ElevatorThread elevator;

void Elevator(int numFloors) {
    DEBUG('H', "\tIn Elevator function...\n");
    int i = 0;
    ASSERT(numFloors > 1);  //an elevator should only exist if there is more
                            //than one floor
    elevator.numFloors = numFloors;
    elevator.currentFloor = 1;
    elevator.numPeopleIn = 0;
    idCounter = 0; //first person will have ID 0
    insideElevatorSemaphore = new Semaphore("inside elevator", ELEVATOR_CAPACITY);
    waitForElevatorSemaphore = new Semaphore("wait for elevator", ELEVATOR_CAPACITY);
    personCreateLock = new Lock("create person");
    elevatorAccessLock = new Lock("elevator access");
    
    while (i++ < ELEVATOR_CAPACITY) {
        elevator.targettedFloors[i] = NO_TARGETTED_FLOOR;
    }
    elevator.targettedFloorIndex = 0;
}

void ArrivingGoingFromTo(int atFloor, int toFloor) {
    DEBUG('H', "\tIn ArrivingGoingFromTo function...\n");
    waitForElevatorSemaphore->P(); //person will request the elevator
    
    int atToArgs = (atFloor*elevator.numFloors) + (toFloor - 1); //transform two
                                                                 //args into one
    Thread *t;
    t = new Thread("forked person thread");
    t->Fork(NewPersonThread, atToArgs);
    
    
    waitForElevatorSemaphore->V(); //person is waiting for the elevator
    
    //elevator arrives at next floor
    
    //CHECK #1
    //is this floor the destination for any person inside the elavator?
    //if yes (check elevator.targettedFloors[]) let that thread wake up and move to CHECK #2
    //if no, go to CHECK #2
    
    //CHECK #2
    //is this floor the targetted floor (pointed to by 
    //elevator.targettedFlors[elevator.targettedFloorIndex]
}

void NewPersonThread(int atToArgs) {
    DEBUG('H', "\tIn NewPersonThread function...\n");
    int i=0;
    int flrIndex=0;;
    int flrValue=0;
    
    PersonThread p;
    p.atFloor = atToArgs / elevator.numFloors;
    p.toFloor = (atToArgs - p.atFloor*elevator.numFloors) + 1;
    
    personCreateLock->Acquire();
    p.id = idCounter++;
    personCreateLock->Release();
    
    //If the circular array elevator.targettedFloorIndex has any open slots,
    //set the next open slot to move towards the waiting person's floor
    elevatorAccessLock->Acquire();
    while (i < elevator.numFloors) {
        flrIndex = (elevator.targettedFloorIndex+i) % elevator.numFloors;
        flrValue = elevator.targettedFloors[flrIndex];
        
        //if the floor the waiting person is on is already targetted, do nothing
        //further and break out of the loop
        if (flrValue == p.atFloor) {
            break;
        } else if (flrValue == NO_TARGETTED_FLOOR) {
            elevator.targettedFloorIndex = flrIndex;
            elevator.targettedFloors[flrIndex] = p.atFloor;
            DEBUG('H', "\t\tAdd waiting person @ elevator.targettedFloors[%d] = %d\n", 
                    elevator.targettedFloorIndex, 
                    elevator.targettedFloors[flrIndex]);
            printf("Person %d wants to go to floor %d from floor %d.\n",
                    p.id, p.toFloor, p.atFloor);
            break;
        }
        i++;
    }
    elevatorAccessLock->Release();
    
    
}

void PerformElevatorAction(int atToArgs) {
    
}

//Move the elevator moves depending on the next targetted floor described by
//elevator.targettedFloors[elevator.targettedFloorIndex]
void MoveElevator() {
    int i = TICKS_TO_NEXT_FLOOR;
    
    while (i-- > 0) { /*do nothing*/ }
    
    elevatorAccessLock->Acquire();
    if (elevator.targettedFloors[elevator.targettedFloorIndex]) {
        elevator.currentFloor++;
    } else {
        elevator.currentFloor--;
    }
    elevatorAccessLock->Release();
}

//#endif //HW1_ELEVATOR

