// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#ifdef CHANGED
#include "pcb.h"
class pcb;
#endif


#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    
    AddrSpace();
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    AddrSpace( const AddrSpace &input);

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code
    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    void setPCB(pcb *input);
    pcb* getPCB();
    int getPID();
    int getNumPages();
    TranslationEntry * getPageTable();
    void AddrSpace::copyMemory(AddrSpace *input);
    bool check();
    void setCheck();
    void setWorked(bool input);
    void SaveReg();
    void RestoreReg();
    void getString(char * str, int virtAd);
    void execThread(OpenFile * executable);
    unsigned int myTranslate(int virtAddr);
    void MapVPN2PPN(int, int);
    
    char* name; //name of address space
    
  private:
    bool worked;
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
    int pageIndex;
    pcb* thisPCB;
    int regArray[NumTotalRegs];
};

#endif // ADDRSPACE_H
