#include "threadPool.h"

int main()
{
	threadPool tp = (threadPool) malloc(sizeof(_threadPool));
	initpool(tp);
	makeJoin(tp);
	freePool(tp);

	exit(EXIT_SUCCESS);
}