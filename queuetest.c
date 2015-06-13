#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <mcheck.h>
#include <unistd.h>

int main()
{
	myQueue myq = (myQueue) malloc(sizeof(_myQueue));
	task t = (task) malloc(sizeof(_task));
	task t2 = (task) malloc(sizeof(_task));

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