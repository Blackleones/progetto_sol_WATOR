#ifndef _QUEUE_
#define _QUEUE_
#include <stdlib.h>
#include <errno.h>
#include "macro.h"

#ifndef DEBUG_QUEUE
#define DEBUG_QUEUE 0
#endif
typedef struct __task _task;
typedef _task* task;

struct __task
{
	/*
		posizione relativa alla KNmatrix
	*/
	int i,j;

	int startX, startY;
	int stopX, stopY;
};

typedef struct __queue _queue;
typedef _queue* queue;

struct __queue 
{
	task mytask;
	queue next;
};

typedef struct __myQueue _myQueue;
typedef _myQueue* myQueue;

/*
	struttura dati principale per gestire la coda
*/
struct __myQueue
{
	int size;
	queue myqueue;
};

/*
	\param myQueue, la coda da inizializzare

	la funzione mette a 0 il puntatore alla testa e setta size = 0
*/
void initMyQueue(myQueue);

/*
	\param myQueue, la coda da esaminare

	\retval 1 se la coda contiene almeno un elemento
	\retval 0 se la coda è vuota
	\retval -1 se la coda non è stata inizializzata
*/
int isEmpty(myQueue);

/*
	\param myQueue, la coda da esaminare

	se la coda non è vuota ritorna il task e decrementa size

	\retval NULL se la coda è vuota
	\retval task t, il primo task della coda
*/
task pop(myQueue);

/*
	\param myQueue, la coda da esaminare
	\param task il task da inserire in cima alla coda

	se la coda non è vuota ritorna il task e incrementa size

	\retval 1 se il task è stato inserito
	\retval -1 se si è verificato un errore, setta errno
*/
int push(myQueue, task);

/*
	\param myQueue, la coda da liberare
*/
void freeQueue(myQueue);

/*
	\param task, il task da inizializzare
	\param startX, startY, stopX, stopY, i valori a cui inizializzare il task
*/
void populateTask(task, int, int, int, int, int, int);

#endif
