#ifndef PCB_H
#define PCB_H 

#include "../threads/thread.h"
class Thread;

#include "../threads/synch.h"
class Lock;


#include "addrspace.h"
class AddrSpace;

#include "pcbManager.h"
class pcbManager;


class pcb {
    public:
	pcb(Thread *input);
	~pcb();
	int  getID();
	AddrSpace* getAddrSpace();
	void setAddrSpace(AddrSpace* input);
	void setParent(pcb * p);
	void addChild(pcb * c);
	void removeChild(int idNum);
	void setThread(Thread *input);
	bool checkForChild(int id);
	pcb * getParent();
	int numberChildren();
	void setChildExitValue(int input);
	int getChildExitValue();
	void setParentsNull();
	Thread* returnThread();

	
    private:
	int MAX_FILES;
	
	Thread *processThread;
	AddrSpace *AdSpace;
	int processID;
	pcb * parent_process;
	pcbManager * children;
	int numChildren;
    int childExitValue;	
};

#endif
