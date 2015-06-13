#ifndef _QUEUE_
#include <stdlib.h>
#include <errno.h>
#include "macro.h"

typedef struct __task _task;
typedef _task* task;

struct __task
{
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

struct __myQueue
{
	int size;
	queue myqueue;
};

int isEmpty(myQueue);
task pop(myQueue);
int push(myQueue, task);

#endif