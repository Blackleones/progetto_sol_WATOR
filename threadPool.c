#include "threadPool.h"

status** initNKmatrix(planet_t* plan)
{
	int i = 0, j = 0;
	int KNnrow = 0;
	int KNncol = 0;
	int planet_nrow = 0;
	int planet_ncol = 0;
	
	status** matrix = NULL;
	
	planet_nrow = plan->nrow;
	planet_ncol = plan->ncol;

	KNnrow = planet_nrow / K + ((planet_nrow % K != 0) ? 1 : 0);
	KNncol = planet_ncol / N + ((planet_ncol % N != 0) ? 1 : 0);

	if(DEBUGTHREAD)
		printf("KNmatrix: %d * %d\n", KNnrow, KNncol);

	matrix = (status**) malloc(KNnrow*sizeof(status*));

	if(matrix == NULL)
	{
		error(EAGAIN, "errore initNKmatrix - creazione KNmatrix");
		return NULL;
	}

	for(i = 0; i < KNnrow; i++)
	{
		matrix[i] = (status*) malloc(KNncol*sizeof(status));

		if(matrix[i] == NULL)
		{
			error(EAGAIN, "errore initNKmatrix - crezione di una riga");
			return NULL;
		}

		/*inizializzo la struttura a DA_ELABORARE*/
		for(j = 0; j < KNncol; j++)
			matrix[i][j] = DA_ELABORARE;
	}

	return matrix;
}

int initpool(threadPool tp, wator_t* w)
{
	int i = 0;
	int checkError = 0;

	if(DEBUGTHREAD)
		printf("entrato in threadPool - initPool\n");

	tp->wator = w;

	/*inizializzo la coda*/
	tp->taskqueue = (myQueue) malloc(sizeof(_myQueue));
	tp->KNmatrix = initNKmatrix(tp->wator->plan);

	if(tp->taskqueue == NULL)
	{
		error(EAGAIN, "errore initpool - malloc taskqueue");
		return -1;
	}

	if(tp->KNmatrix == NULL)
	{
		error(EAGAIN, "errore initpool - malloc KNmatrix");
		return -1;		
	}

	initMyQueue(tp->taskqueue);

	tp->run = 1;
	tp->workFlag = 0;
	tp->collectorFlag = 0;
	tp->workingThread = 0;

	/*
		inizializzo mutex e cw
	*/
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

	/*
		inizializzo dispatcher
	*/
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

	while(tp->run)
	{

		if(DEBUGTHREAD)
			printf("---------DISPATCHER---------\n");

		/*
			acquisisco la lock ed inserisco i task da elaborare
		*/
		pthread_mutex_lock(&(tp->queueLock));
		
		while(tp->workFlag != 0 || isEmpty(tp->taskqueue))
			pthread_cond_wait(&(tp->waitingCollector), &(tp->queueLock));

		task t1 = (task) malloc(sizeof(_task));
		task t2 = (task) malloc(sizeof(_task));
		task t3 = (task) malloc(sizeof(_task));
		task t4 = (task) malloc(sizeof(_task));
		task t5 = (task) malloc(sizeof(_task));
		task t6 = (task) malloc(sizeof(_task));

		push(taskqueue, t1);
		push(taskqueue, t2);
		push(taskqueue, t3);
		push(taskqueue, t4);
		push(taskqueue, t5);
		push(taskqueue, t6);
		
		tp->workFlag = 1;
		pthread_cond_broadcast(&(tp->waitingDispatcher));
		pthread_mutex_unlock(&(tp->queueLock));
	}

	pthread_exit(EXIT_SUCCESS);
}

void* workerTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;

	while(tp->run)
	{

		if(DEBUGTHREAD)
			printf("---------WORKER---------\n");

		/*
			acquisisco la lock
				-se la coda è vuota estraggo un task
			rilascio la lock
				-elaboro il task

			<da fare>
			acquisisco la lock sulla matrice
				-controllo i vicini
			rilascio la lock
				-elaboro
		*/
		pthread_mutex_lock(&(tp->queueLock));

		while(tp->workFlag == 0 || !isEmpty(tp->taskqueue))
			pthread_cond_wait(&(tp->waitingDispatcher), &(tp->queueLock));

		pop(taskqueue);

		if(isEmpty(taskqueue) == 0){
			tp->collectorFlag = 1;
			pthread_cond_signal(&(tp->waitingWorkers));
		}

		pthread_mutex_unlock(&(tp->queueLock));
	}

	pthread_exit(EXIT_SUCCESS);
}

void* collectorTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;

	while(tp->run)
	{

		if(DEBUGTHREAD)
			printf("---------COLLECTOR---------\n");

		pthread_mutex_lock(&(tp->queueLock));

		if(DEBUGTHREAD)
			printf("SIZE = %d\n", taskqueue->size);
		
		/*
			isEmpty è commentato perche collectorFlag viene settato a 1 quando 
			taskqueue->size = 0 => isEmpty ritorna sicuramente 1 perche viene chiamata sotto
			lock
		*/
		while(tp->collectorFlag == 0 || tp->workingThread != 0/* || isEmpty(taskqueue)*/)
			pthread_cond_wait(&(tp->waitingWorkers), &(tp->queueLock));

		tp->collectorFlag = 0;
		tp->workFlag = 0;
		pthread_cond_signal(&(tp->waitingCollector));
		pthread_mutex_unlock(&(tp->queueLock));
	}

	pthread_exit(EXIT_SUCCESS);
}