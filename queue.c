#include "queue.h"

int isEmpty(myQueue myq)
{
	if(DEBUGQUEUE)
		printf("entrato in queue - isEmpty\n");

	if(myq != NULL)
		return (myq->size == 0) ? 0 : 1;
	else
		return -1;
}

void initMyQueue(myQueue myq)
{
	if(DEBUGQUEUE)
		printf("entrato in queue - initMyQueue\n");

	myq->myqueue = NULL;
	myq->size = 0;
}

int push(myQueue myq, task t)
{
	queue q = (queue) malloc(sizeof(_queue));

	if(DEBUGQUEUE)
		printf("entrato in queue - push\n");

	if(q == NULL)
	{
		error(EAGAIN, "errore in push");
		return -1;
	}

	q->mytask = t;
	q->next = myq->myqueue;
	myq->myqueue = q;
	myq->size++;

	return 1;
}

task pop(myQueue myq)
{
	queue q = myq->myqueue;
	task t = NULL;

	if(DEBUGQUEUE)
		printf("entrato in queue - pop\n");

	if(q == NULL)
		return NULL;

	t = q->mytask;
	myq->myqueue = q->next;

	free(q);
	myq->size--;

	return t;
}

void populateTask(task t, int sX, int sY, int fX, int fY)
{
	if(DEBUGQUEUE)
		printf("entrato in queue - populateTask\n");

	t->startX = sX;
	t->startY = sY;
	t->stopX = fX;
	t->stopY = fY;
}

void freeQueue(myQueue myq)
{
	queue q = NULL;
	queue p = NULL;

	if(DEBUGQUEUE)
		printf("entrato in queue - freeQueue\n");

	if(myq != NULL)
	{
		q = myq->myqueue;

		while(q != NULL)
		{
			p = q;
			q = q->next;

			free(p->mytask);
			free(p);
		}
	}

}