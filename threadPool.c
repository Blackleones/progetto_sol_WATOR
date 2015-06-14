#include "threadPool.h"

int initpool(threadPool tp)
{
	int i = 0;
	int checkError = 0;

	if(DEBUGTHREAD)
		printf("entrato in threadPool - initPool\n");

	/*inizializzo la coda*/
	tp->taskqueue = (myQueue) malloc(sizeof(_myQueue));

	if(tp->taskqueue == NULL)
	{
		error(EAGAIN, "errore initpool - malloc taskqueue");
		return -1;
	}

	initMyQueue(tp->taskqueue);

	tp->run = 1;
	tp->work = 0;
	tp->workingThread = 0;

	/*inizializzo dispatcher*/
	checkError = pthread_create(&(tp->dispatcher), NULL, dispatcherTask, (void*) &tp);

	if(checkError != 0)
	{
		error(EAGAIN, "errore initpool - creazione dispatcher");
		return -1;
	}

	/*inizializzo collector*/
	checkError = pthread_create(&(tp->collector), NULL, collectorTask, (void*) &tp);

	if(checkError != 0)
	{
		error(EAGAIN, "errore initpool - creazione collector");
		return -1;
	}

	/*inizializzo i worker*/
	tp->workers = (pthread_t*) malloc(NWORK_DEF*sizeof(pthread_t));

	if(tp->workers == NULL)
	{
		error(EAGAIN, "errore initpool - malloc workers");
		return -1;
	}

	for(i = 0; i < NWORK_DEF; i++)
	{
		checkError = pthread_create(&(tp->workers[i]), NULL, workerTask, (void*) &tp);

		if(checkError != 0)
		{
			error(checkError, "errore initpool - creazione i-esimo worker thread");
			return -1;
		}

	}

	checkError = pthread_mutex_init(&(tp->queueLock), NULL);

	if(checkError != 0)
	{
		error(checkError, "errore initpool - inizializzazione lock");
		return -1;
	}

	checkError = pthread_cond_init(&(tp->waitingDispatcher), NULL);

	if(checkError != 0)
	{
		error(checkError, "errore initpool - inizializzazione cw waitingDispatcher");
		return -1;
	}

	checkError = pthread_cond_init(&(tp->waitingWorkers), NULL);

	if(checkError != 0)
	{
		error(checkError, "errore initpool - inizializzazione cw waitingWorkers");
		return -1;
	}

	checkError = pthread_cond_init(&(tp->waitingCollector), NULL);

	if(checkError != 0)
	{
		error(checkError, "errore initpool - inizializzazione cw waitingCollector");
		return -1;
	}

	return 1;
}

int makeJoin(threadPool tp)
{
	int i = 0;
	int checkError = 0;
	/*
		faccio le join su tutti i thread <deve andare nel main>
	*/
	checkError = pthread_join(tp->dispatcher, NULL);
	checkError = pthread_join(tp->collector, NULL);

	for(i = 0; i < NWORK_DEF; i++)
	{
		checkError = pthread_join(tp->workers[i], NULL);
		
		if(checkError != 0)
		{
			error(checkError, "errore startPool - join i-esimo worker thread");
			return -1;
		}
	}	

	return 1;
}

void freePool(threadPool tp)
{
	if(DEBUGTHREAD)
		printf("entrato in threadPool - freePool\n");

	freeQueue(tp->taskqueue);
}

void* dispatcherTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;

	if(DEBUGTHREAD)
		printf("entrato in threadPool - dispatcherTask\n");	

	while(0)
	{

	}

	pthread_exit(EXIT_SUCCESS);
}

void* workerTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;

	if(DEBUGTHREAD)
		printf("entrato in threadPool - workerTask\n");

	while(0)
	{

	}

	pthread_exit(EXIT_SUCCESS);
}

void* collectorTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;

	if(DEBUGTHREAD)
		printf("entrato in threadPool - collectorTask\n");

	while(0)
	{

	}

	pthread_exit(EXIT_SUCCESS);
}