// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void updateCounter();
void syscallYield();
void syscallExit();
int syscallJoin();
int syscallFork();
void helpFork(int i);
int syscallKill();
void syscallHalt();
int syscallExec();
void handlePageFaultException();

#if defined(CHANGED)

void
ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if (which == PageFaultException) {
        DEBUG('D', "PageFaultException being handled\n");
        handlePageFaultException();
    }

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        syscallHalt();
    }
    else if ((which == SyscallException) && (type == SC_Yield)) {
        DEBUG('a', "Yield System Call.\n");
        syscallYield();
        updateCounter();
    } else if ((which == SyscallException) && (type == SC_Exit)) {
        DEBUG('a', "Exit System Call.\n");
        syscallExit();
    } else if ((which == SyscallException) && (type == SC_Fork)) {
        DEBUG('a', "Fork System Call.\n");
        syscallFork();
        updateCounter();
    } else if ((which == SyscallException) && (type == SC_Exec)) {
        DEBUG('a', "Exec System Call.\n");
        syscallExec();
    } else if ((which == SyscallException) && (type == SC_Join)) {
        DEBUG('a', "Join System Call.\n");
        syscallJoin();
        updateCounter();
    } else if ((which == SyscallException) && (type == SC_Kill)) {
        DEBUG('a', "Kill System Call.\n");
        syscallKill();
        updateCounter();
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
#else

void
ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
#endif


#include "../machine/machine.h"

extern Machine* machine;
Lock* pagingLock;

//#if defined(CHANGED)

void handlePageFaultException() {
    if (pagingLock == NULL) {
        pagingLock = new Lock("pageingLock");
    }

    int badVirtualAddr = machine->ReadRegister(BadVAddrReg);
    pagingLock->Acquire();
    currentThread->space->pageFault(vpn);
    pagingLock->Release();
}

void updateCounter() {
    int counter;
    counter = machine->ReadRegister(PCReg);
    counter += 4;
    machine->WriteRegister(PrevPCReg, counter - 4);
    machine->WriteRegister(PCReg, counter);
    machine->WriteRegister(NextPCReg, counter + 4);
}

int syscallExec() {

    printf("Syscall Call: [%d] invoked Exec.\n", currentThread->space->getPID());

    //get the string an copy to memory of this addrspace
    int virtualAd = machine->ReadRegister(4);
    char * fileName = new char[256];
    currentThread->space->getString(fileName, virtualAd);


    OpenFile *fileRead = fileSystem->Open(fileName);
    if (fileRead == NULL) {
        printf("Unable to open file %s\n", fileName);
        updateCounter();
        machine->WriteRegister(2, -1);
        return -1;
    } else {
        currentThread->space->execThread(fileRead);

        if (currentThread->space->check() == false) {
            updateCounter();
            return -1;
        }

        printf("Exec Program: [%d] loading [%s]\n", currentThread->space->getPID(), fileName);
        machine->WriteRegister(2, 1);
        return 1;
    }
}

void syscallHalt() {
    printf("Syscall Call: [%d] invoked call Halt.\n", currentThread->space->getPID());
    interrupt->Halt();
}

void syscallYield() {
    printf("System Call: [%d] invoked Yield.\n", currentThread->space->getPID());
    currentThread->Yield();
}

int syscallKill() {
    int i, killID, index;
    pcb* killPCB;
    printf("System Call: [%d] invoked Kill.\n", currentThread->space->getPID());


    killID = machine->ReadRegister(4);

    //if its not a valid ID, return error
    if (!pcbMan->validPID(killID)) {
        printf("Process [%d] cannot kill process [%d]: doesn't exist\n", currentThread->space->getPID(), killID);
        machine->WriteRegister(2, -1);
        return -1;
    } else if (currentThread == (pcbMan->getThisPCB(killID))->returnThread()) {
        syscallExit();
        machine->WriteRegister(2, 0);
        index = killID;
        return 0;
    } else {

        //get a pointer to the pcb of the process to be killed
        killPCB = pcbMan->getThisPCB(killID);
        index = killID;

        //if this process has children, set their parent pointers to null
        if (killPCB->numberChildren() > 0) {
            killPCB->setParentsNull();
        }

        //if this process has a parent, remove itself from the childManager  

        if (killPCB->getParent() != NULL) {
            killPCB->getParent()->removeChild(killID);
        }

        //remove itself from the pcbManager and PID manager
        pcbMan->removePCB(killID);
        pid_manager->removePid(killID);

        //free up the memory
        AddrSpace *tempAd = killPCB->getAddrSpace();
        TranslationEntry *tempPage = tempAd->getPageTable();
        for (i = 0; i < tempAd->getNumPages(); i++) {
            mans_man->deallocate(tempPage[i].physicalPage);
        }

        //check open files and delete them 

        //Remove Thread from Scheduler and delete it
        Thread* killThread = killPCB->returnThread();
        scheduler->RemoveThisThread(killThread);
        index = 0;
        delete tempAd;

        printf("Process [%d] killed process [%d]\n", currentThread->space->getPID(), killID);
        machine->WriteRegister(2, 0);
        return 0;
    }

}

void syscallExit() {

    int i, id, exVal, index;
    printf("System Call: [%d] invoked Exit.\n", currentThread->space->getPID());
    exVal = machine->ReadRegister(4);

    //if this process has children, set their parent pointers to null
    if (currentThread->space->getPCB()->numberChildren() > 0) {
        currentThread->space->getPCB()->setParentsNull();
        index = exVal;
    }

    //if this process has a parent, remove itself from the childManager and set exit value to parent 
    id = currentThread->space->getPID();


    if (currentThread->space->getPCB()->getParent() != NULL) {
        currentThread->space->getPCB()->getParent()->removeChild(id);
        currentThread->space->getPCB()->getParent()->setChildExitValue(exVal);
        index = id;
    }

    //remove itself from the pcbManager and PID manager
    pcbMan->removePCB(id);
    pid_manager->removePid(id);

    AddrSpace *tempAd = currentThread->space;
    TranslationEntry *tempPage = tempAd->getPageTable();
    for (i = 0; i < tempAd->getNumPages(); i++) {
        mans_man->deallocate(tempPage[i].physicalPage);
    }

    printf("Process [%d] exits with [%d]\n", id, exVal);

    //if this is the last process, then just exit
    delete tempAd;
    currentThread->Finish();
}

int syscallJoin() {
    int id;
    int index;
    printf("System Call: [%d] invoked Join.\n", currentThread->space->getPID());
    id = machine->ReadRegister(4);

    if (currentThread->space->getPCB()->getParent() != NULL) {
        if (currentThread->space->getPCB()->getParent()->getID() == id) {
            machine->WriteRegister(2, -1);
            return -1;
        }
    } else if (currentThread->space->getPCB()->checkForChild(id)) {
        index = id;
        while (currentThread->space->getPCB()->checkForChild(id)) {
            currentThread->Yield();
        }
        machine->WriteRegister(2, currentThread->space->getPCB()->getChildExitValue());
        return (currentThread->space->getPCB()->getChildExitValue());
    } else {
        machine->WriteRegister(2, -1);
    }

    return -1;

}

void helpFork(int i) {
    currentThread->space->RestoreReg();
    currentThread->space->RestoreState();
    machine->Run();
}

int syscallFork() {

    unsigned int oldPC, oldPrevPC, oldNextPC;
    unsigned int index;
    currentThread->space->SaveReg();

    printf("System Call: [%d] invoked Fork.\n", currentThread->space->getPID());

    AddrSpace *tempAd = new AddrSpace();

    Thread *t = new Thread("ForksThread");
    t->space = tempAd;
    tempAd->getPCB()->setThread(t);

    currentThread->space->copyMemory(tempAd);

    //if there isn't enough memory for the child process, check will return false, and fork will quit
    if (!tempAd->check()) {
        machine->WriteRegister(2, -1);
        return -1;
    }

    memLock->Acquire();
    oldPC = machine->ReadRegister(PCReg);
    oldPrevPC = machine->ReadRegister(PrevPCReg);
    oldNextPC = machine->ReadRegister(NextPCReg);
    index = oldPC;
    machine->WriteRegister(PCReg, machine->ReadRegister(4));
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(4) - 4);
    machine->WriteRegister(NextPCReg, machine->ReadRegister(4) + 4);

    tempAd->SaveReg();
    t->Fork(helpFork, 1);

    machine->WriteRegister(PCReg, oldPC);
    machine->WriteRegister(PrevPCReg, oldPrevPC);
    machine->WriteRegister(NextPCReg, oldNextPC);

    printf("Process [%d] Fork: start at address [0x%x] with [%d] pages memory\n", currentThread->space->getPID(), machine->ReadRegister(4), currentThread->space->getNumPages());


    currentThread->space->RestoreReg();

    machine->WriteRegister(2, tempAd->getPID());
    memLock->Release();
    return tempAd->getPID();
}


//#endif
