#include "syscall.h"

void usememory(){
	Exec("/home/bear-b/users/ywang032/TA/testcases/memory");
}

int main()
{
	int i=0;

	for (i = 0; i < 5; i++) {
		Fork(usememory);
		Yield();
	}
	Exit(0);
}

