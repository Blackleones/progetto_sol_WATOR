#include "threadPool.h"
/*
gcc -g -Wall -pedantic -pthread -DDEBUGQUEUE -DDEBUGTHREAD -o t threadpoolTest.c threadPool.c queue.c 
*/
int main()
{
	threadPool tp = (threadPool) malloc(sizeof(_threadPool));
	initpool(tp);
	makeJoin(tp);
	freePool(tp);

	exit(EXIT_SUCCESS);
}