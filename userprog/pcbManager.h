#ifndef PCBMANAGER_H
#define PCBMANAGER_H

#include "pcb.h"
class pcb;

class pcbManager
{

    public: 
	pcbManager();
	void assignPCB(pcb *input);
	void removePCB(int pid);
	int getNumPCB();
	bool validPID(int pid); 
	void setParentNull();
	pcb* getThisPCB(int pcbID);

    private:
    	pcb *pcbArray[32];
	bool usage[32];
	int pcb_count;
    int currPid;
};
#endif
