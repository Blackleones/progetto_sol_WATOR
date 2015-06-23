#include "threadPool.h"

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

static volatile int flag_alarm = 0;
static volatile int flag_check = 0;
static volatile int flag_create = 0;
static volatile int flag_close = 0;

static void set_alarm()
{
	if(flag_alarm == 0)
	{
		flag_create = 1;
		flag_alarm = 1;
		alarm(SECS);
	}
}

static void set_check()
{
	flag_check = 1;
	alarm(SECS);
}

static void set_close()
{
	flag_close = 1;
}

void* signalTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	FILE* filecheck = NULL;

	while(tp->run)
	{
		if(flag_create)
		{
			flag_create = 0;
			filecheck = fopen(WATOR_CHECK, "w");

			if(filecheck == NULL)
			{
				perror("initpool - errore crezione filecheck");
				pthread_exit((void*) -1);
			}
		}

		if(flag_check)
		{
			flag_check = 0;

			fseek(filecheck, 0, SEEK_SET);
			print_planet(filecheck, tp->wator->plan);
		}

		if(flag_close)
			tp->close = 1;
	}

	pthread_exit(0);
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
	tp->close = 0;

	loadFlagMap(tp->wator->plan, tp->flagMap);
	loadKNM(tp->KNM);

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
	tp->workers = (pthread_t*) malloc((tp->wator->nwork)*sizeof(pthread_t));

	if(tp->workers == NULL)
	{
		error(EAGAIN, "errore initpool - malloc workers");
		return -1;
	}

	for(i = 0; i < tp->wator->nwork; i++)
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

	/*
		inizializzo gestore dei segnali
	*/
	sigset_t set;
	struct sigaction usr1;
	struct sigaction sint;
	struct sigaction term;
	struct sigaction salarm;
	ec_meno1(sigfillset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

	bzero(&usr1, sizeof(usr1));
	bzero(&sint, sizeof(sint));
	bzero(&term, sizeof(term));
	bzero(&salarm, sizeof(salarm));

	usr1.sa_handler = set_alarm;
	sint.sa_handler = set_close;
	term.sa_handler = set_close;
	salarm.sa_handler = set_check;

	ec_meno1(sigaction(SIGUSR1, &usr1, NULL));
	ec_meno1(sigaction(SIGINT, &sint, NULL));
	ec_meno1(sigaction(SIGTERM, &term, NULL));
	ec_meno1(sigaction(SIGALRM, &salarm, NULL));

	pthread_create(&(tp->signal_handler), NULL, signalTask, (void*)  &tp);

	ec_meno1(sigemptyset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

	return 1;
}

void freePool(threadPool tp)
{
	int i = 0;

	if(DEBUG_THREAD)
		printf("entrato in threadPool - freePool\n");

	freeQueue(tp->taskqueue);
	
	/*
		libero la KNmatrix
	*/
	for(i = 0; i < tp->KNM->nrow; i++)
		free(tp->KNM->matrix[i]);

	free(tp->KNM->matrix);
	free(tp->KNM);
	
	/*
		libero flagMap
	*/
	for(i = 0; i < tp->wator->plan->nrow; i++)
		free(tp->flagMap[i]);

	free(tp->flagMap);

	/*
		libero wator
	*/
	free_wator(tp->wator);

	/*
		libero le mutex e cw
	*/
	pthread_mutex_destroy(&(tp->queueLock));
	pthread_mutex_destroy(&(tp->KNMLock));
	pthread_cond_destroy(&(tp->waitingCollector));
	pthread_cond_destroy(&(tp->waitingDispatcher));
	pthread_cond_destroy(&(tp->waitingWorkers));
	pthread_cond_destroy(&(tp->waitingTask));
}

int makeJoin(threadPool tp)
{
	int i = 0;
	int checkError = 0;
	/*
		faccio le join su tutti i thread <devo inserire il flag di chiusura>
	*/
	checkError = pthread_join(tp->dispatcher, NULL);
	checkError = pthread_join(tp->collector, NULL);
	checkError = pthread_join(tp->signal_handler, NULL);
	for(i = 0; i < tp->wator->nwork; i++)
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
