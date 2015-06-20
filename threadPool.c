#include "threadPool.h"

static void stampa(planet_t* planet)
{
	system("clear");
	printf("\n");
	//sleep(1);
	int i = 0, j = 0; 
	int nrow = planet->nrow;
	int ncol = planet->ncol;
	cell_t** map = planet->w;

	for(i = 0; i < nrow; i++)
	{
		for(j = 0; j < ncol-1; j++)
			printf("%c ", cell_to_char(map[i][j]));

		printf("%c", cell_to_char(map[i][j]));
		printf("\n");
	}
}

static void printFlagMap(int** flagMap, int nrow, int ncol, char* message)
{
	int i = 0, j = 0;

	printf("\n|||||== %s: FLAGMAP ==|||||\n", message);

	for(i = 0; i < nrow; i++)
	{
		for(j = 0; j < ncol; j++)
			printf("%d ", flagMap[i][j]);

		printf("\n");
	}	
}

void printTask(task t)
{
	printf("\n=============================================\n");
	printf("=\t[%d][%d]:\n=\t\tV: %d -> %d\n=\t\tO: %d -> %d\n=", t->i, t->j, t->startY, t->stopY, t->startX, t->stopX);
	printf("\n=============================================\n");
}

void printKNM(KNmatrix knm, char* message)
{
	int i = 0, j = 0;

	printf("\n=======================%s======================\n", message);
	printf("	KN MATRIX					\n");
	for(i = 0; i < knm->nrow; i++){
		for(j = 0; j < knm->ncol; j++)
			printf("%d ", knm->matrix[i][j]);

		printf("\n");
	}
	printf("\n=============================================\n");
}

void populateQueue(threadPool tp)
{
	int i = 0, j = 0;
	int cri = 0, crf = 0;
	int cci = 0, ccf = 0;

	int KNrow =	tp->KNM->nrow;
	int KNcol = tp->KNM->ncol;
	int nrow = tp->wator->plan->nrow;
	int ncol = tp->wator->plan->ncol;


	while(i < KNrow)
	{
		crf = ((cri + K - 1 < nrow) ? cri + K - 1 : nrow -1);
		cci = 0;
		ccf = 0;
		while(j < KNcol)
		{
			ccf = ((ccf + N - 1 < ncol) ? ccf + N - 1 : ncol-1);
			
			task t = (task) malloc(sizeof(_task));
			populateTask(t, i, j, cri, cci, crf, ccf);
			push(tp->taskqueue, t);

			cci = ((ccf + 1 < ncol) ? ccf + 1 : ncol - 1);
			ccf++;
			j++;
		}

		cri = ((crf + 1 < nrow) ? crf + 1 : nrow - 1);
		j = 0;
		i++;
	}
}

KNmatrix initNKmatrix(planet_t* plan)
{
	int i = 0, j = 0;
	int KNnrow = 0;
	int KNncol = 0;
	int planet_nrow = 0;
	int planet_ncol = 0;
	
	KNmatrix knm = (KNmatrix) malloc(sizeof(_KNmatrix));

	status** matrix = NULL;
	
	planet_nrow = plan->nrow;
	planet_ncol = plan->ncol;

	KNnrow = planet_nrow / K + ((planet_nrow % K != 0) ? 1 : 0);
	KNncol = planet_ncol / N + ((planet_ncol % N != 0) ? 1 : 0);

	if(DEBUG_THREAD)
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
			matrix[i][j] = WAITING;
	}

	knm->nrow = KNnrow;
	knm->ncol = KNncol;
	knm->matrix = matrix;

	return knm;
}

void loadKNM(KNmatrix knm)
{
	int i = 0, j = 0;

	for(i = 0; i < knm->nrow; i++)
		for(j = 0; j < knm->ncol; j++)
			knm->matrix[i][j] = WAITING;

}

int checkMutex(KNmatrix knm, int i, int j)
{
	int nrow = knm->nrow;
	int ncol = knm->ncol;
	status** matrix = knm->matrix;

	return matrix[mod(i+1, nrow)][j] != RUNNING 
		&& matrix[mod(i-1, nrow)][j] != RUNNING
		&& matrix[i][mod(j+1, ncol)] != RUNNING
		&& matrix[i][mod(j-1, ncol)] != RUNNING;
}

void loadFlagMap(planet_t* plan, int** flagMap)
{
	cell_t** map = plan->w;
	int i = 0, j = 0;
	int nrow = plan->nrow;
	int ncol = plan->ncol;

	for(i = 0; i < nrow; i++)
		for(j = 0; j < ncol; j++)
			if(map[i][j] != WATER)
				flagMap[i][j] = MOVE;
}

int evolve(task t, wator_t* pw, int** flagMap)
{
	int i = 0, j = 0;
	int k = 0, l = 0;
	int action = 0;
	cell_t** map = pw->plan->w;

	for(i = t->startX; i <= t->stopX; i++)
	{
		for(j = t->startY; j <= t->stopY; j++)
		{
			if(flagMap[i][j] == MOVE && map[i][j] == SHARK)
			{
				action = shark_rule2(pw, i, j, &k, &l);

				if(action == -1)
					return -1;

				if(action != DEAD)
				{
					action = shark_rule1(pw, i, j, &k, &l);

					if(action == -1)
						return -1;

					if(action == EAT || action == MOVE)
						flagMap[k][l] = STOP;
				}
			}

			if(flagMap[i][j] == MOVE && map[i][j] == FISH)
			{
				action = fish_rule4(pw, i, j, &k, &l);

				if(action == -1)
					return -1;

				action = fish_rule3(pw, i, j, &k, &l);

				if(action == -1)
					return -1;

				if(action == MOVE)
					flagMap[k][l] = STOP;

				/*flagMap[i][j] = STOP;*/
			}
		}
	}

	return 1;
}

static int** initFlagMap(int nrow, int ncol)
{
	int i = 0;
	int** flagMap = (int**) calloc(nrow, sizeof(int*));

	if(flagMap == NULL)
		return NULL;

	for(i = 0; i < nrow; i++)
	{
		flagMap[i] = (int*) calloc(ncol, sizeof(int));

		if(flagMap[i] == NULL)
			return NULL;
	}

	return flagMap;
}

int initpool(threadPool tp, wator_t* w)
{
	int i = 0;
	int checkError = 0;

	if(DEBUG_THREAD)
		printf("entrato in threadPool - initPool\n");

	tp->wator = w;

	/*
		inizializzo la coda
	*/
	tp->taskqueue = (myQueue) malloc(sizeof(_myQueue));

	if(tp->taskqueue == NULL)
	{
		error(EAGAIN, "errore initpool - malloc taskqueue");
		return -1;
	}

	/*
		inizializzo la matrice di supporto ai thread
	*/
	tp->KNM = initNKmatrix(tp->wator->plan);

	if(tp->KNM == NULL)
	{
		error(EAGAIN, "errore initpool - malloc KNmatrix");
		return -1;		
	}

	/*
		inizializzo la matrice di supporto degli spostamenti
	*/
	tp->flagMap = initFlagMap(tp->wator->plan->nrow, tp->wator->plan->ncol);

	if(tp->flagMap == NULL)
	{
		error(EAGAIN, "errore initpool - calloc flagMap");
		return -1;
	}

	/*
		inizializzo la struttura che gestisce la coda
	*/
	initMyQueue(tp->taskqueue);

	/*
		setto i flag per gestire i thread
	*/
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

	checkError = pthread_mutex_init(&(tp->KNMLock), NULL);

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

	checkError = pthread_cond_init(&(tp->waitingTask), NULL);

	if(checkError != 0)
	{
		error(checkError, "errore initpool - inizializzazione cw waitingTask");
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
		workerargs wa = (workerargs) malloc(sizeof(_workerargs));
		wa->n = i;
		wa->tp = tp;

		checkError = pthread_create(&(tp->workers[i]), NULL, workerTask, (void*) &wa);

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
	if(DEBUG_THREAD)
		printf("entrato in threadPool - freePool\n");

	freeQueue(tp->taskqueue);
	/*liberare tutte le colonne*/
	free(tp->KNM->matrix);
	free(tp->KNM);
	free_wator(tp->wator);
}

void* dispatcherTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);

	while(tp->run)
	{
		loadFlagMap(tp->wator->plan, tp->flagMap);
		loadKNM(tp->KNM);
		
		if(DEBUG_THREAD_MATRIX)
		{
			printFlagMap(tp->flagMap, tp->wator->plan->nrow, tp->wator->plan->ncol, "DISPATCHER:");
			printKNM(tp->KNM, "DISPATCHER:");
		}

		if(DEBUG_THREAD)
			printf("---------DISPATCHER---------\n");

		/*
			acquisisco la lock ed inserisco i task da elaborare
		*/
		pthread_mutex_lock(&(tp->queueLock));
		
		while(tp->workFlag != 0 || isEmpty(tp->taskqueue))
			pthread_cond_wait(&(tp->waitingCollector), &(tp->queueLock));

		/*inserisco tutti i task*/
		populateQueue(tp);

		tp->workFlag = 1;
		pthread_cond_broadcast(&(tp->waitingDispatcher));
		pthread_mutex_unlock(&(tp->queueLock));
	}

	pthread_exit(EXIT_SUCCESS);
}

void* workerTask(void* _wa)
{
	workerargs wa = *((workerargs*) _wa);
	int n = wa->n;
	threadPool tp = wa->tp;
	myQueue taskqueue = tp->taskqueue;
	int** flagMap = tp->flagMap;

	while(tp->run)
	{

		if(DEBUG_THREAD)
			printf("---------WORKER---------\n");

		/*
			acquisisco la lock
				-se la coda è vuota estraggo un task
			rilascio la lock
		*/
		pthread_mutex_lock(&(tp->queueLock));

		while(tp->workFlag == 0 || !isEmpty(tp->taskqueue))
			pthread_cond_wait(&(tp->waitingDispatcher), &(tp->queueLock));

		task t = pop(taskqueue);

		if(DEBUG_THREAD_TASK)
			printTask(t);
		pthread_mutex_unlock(&(tp->queueLock));

		/*
			acquisisco la lock
				-mi metto in attesase un thread sta elaborando una porzione vicino al mio task
			rilascio la lock
			elaboro
		*/
		pthread_mutex_lock(&(tp->KNMLock));
		(tp->workingThread)++;

		if(DEBUG_THREAD_MATRIX)
			printKNM(tp->KNM, "WORKER:");

		while(checkMutex(tp->KNM, t->i, t->j) == 0)
			pthread_cond_wait(&(tp->waitingTask), &(tp->KNMLock));

		tp->KNM->matrix[t->i][t->j] = RUNNING;
		pthread_mutex_unlock(&(tp->KNMLock));

		evolve(t, tp->wator, flagMap);

		pthread_mutex_lock(&(tp->KNMLock));
		(tp->workingThread--);
		tp->KNM->matrix[t->i][t->j] = DONE;

		/*se la cosa è vuota sveglio il collector*/
		if(isEmpty(taskqueue) == 0){
			tp->collectorFlag = 1;
			pthread_cond_signal(&(tp->waitingWorkers));
		}

		/*sveglio tutti i worker in attesa per elaborare il task*/
		pthread_cond_broadcast(&(tp->waitingTask));
		pthread_mutex_unlock(&(tp->KNMLock));
	}

	pthread_exit(EXIT_SUCCESS);
}

void* collectorTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;
	volatile int currentChronon = 0;

	while(tp->run)
	{

		if(DEBUG_THREAD)
			printf("---------COLLECTOR---------\n");

		pthread_mutex_lock(&(tp->queueLock));

		if(DEBUG_THREAD_TASK)
			printKNM(tp->KNM, "COLLECTOR");

		if(DEBUG_THREAD)
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

		if(DEBUG_THREAD_MATRIX)
		{
			printKNM(tp->KNM, "COLLECTOR:");
			printFlagMap(tp->flagMap, tp->wator->plan->nrow, tp->wator->plan->ncol, "COLLECTOR");
		}

		if(currentChronon % tp->wator->chronon)
		{
			stampa(tp->wator->plan);
			currentChronon = 0;
		}
		else
			currentChronon++;

		pthread_cond_signal(&(tp->waitingCollector));
		pthread_mutex_unlock(&(tp->queueLock));
	}

	pthread_exit(EXIT_SUCCESS);
}