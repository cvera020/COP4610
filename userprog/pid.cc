#include "pid.h"

pid::pid()
{
    int i;
    for(i=1; i<32; i++) 
    {
	ids[i] = false;
    }
}

int pid::getPid()
{
    int i;

    for(i=1; i<32; i++)
    {
	if(ids[i] == false)
	{
	    ids[i] = true;
	    return i;
	}
    }
    return -1;
}

void pid::removePid(int input)
{
    ids[input] = false;
}
