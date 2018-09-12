#include "machine.h"
class memMan
{
    public:

	memMan();
	int allocate();
	void deallocate(int pageNum);
	int getPages();
 
    private:
    	bool mem[32];
	int numPages;
    int pageIndex;

};
