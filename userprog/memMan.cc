#include "memMan.h"

memMan:: memMan()
{
    int i;
    for(i=0; i<32; i++)
    { 
	mem[i]=false;
    }
}

int memMan:: allocate()
{
    int i;
    for(i=0; i<32; i++)
    {
	if(mem[i]==false)
	{
	    mem[i]=true;
	    numPages++;
        pageIndex++;
	    return i;
	}
    }
    return -1;
}

void memMan:: deallocate(int pageNum)
{
    mem[pageNum]=false;
    numPages--;
    pageIndex--;
}

int memMan:: getPages()
{
    return (NumPhysPages-numPages);
}
