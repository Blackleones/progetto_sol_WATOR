#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <mcheck.h>
#include <unistd.h>

void stampaT(task t)
{
	printf("%d %d %d %d\n", t->startX, t->startY, t->stopX, t->stopY);
}

int main()
{
	myQueue myq = (myQueue) malloc(sizeof(_myQueue));
	task t = (task) malloc(sizeof(_task));
	task t2 = (task) malloc(sizeof(_task));

	system("clear");

	populateTask(t, 1, 1, 2, 2);
	populateTask(t2, 1, 2, 3, 4);

	stampaT(t);
	stampaT(t2);

	if(isEmpty(myq) != 0)
	{
		printf("errore isempty != 0 con coda vuota\n");
		exit(EXIT_FAILURE);
	}

	push(myq, t);

	if(isEmpty(myq) != 1)
	{
		printf("errore isempty != 1 con coda non vuota\n");
		exit(EXIT_FAILURE);
	}

	push(myq, t);
	push(myq, t2);

	if(pop(myq) != t2)
	{
		printf("l'elemento ritornato non è corretto (t2)\n");
		exit(EXIT_FAILURE);
	}

	if(pop(myq) != t)
	{
		printf("l'elemento ritornato non è corretto (t)\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
