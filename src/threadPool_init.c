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

	/*
		la matrice KNmatrix è una rappresentazione in "scala" del pianeta
	*/
	KNnrow = planet_nrow / K + ((planet_nrow % K != 0) ? 1 : 0);
	KNncol = planet_ncol / N + ((planet_ncol % N != 0) ? 1 : 0);

	if(DEBUG_THREAD)
		printf("%sKNmatrix ha dimensione: %d * %d%s\n", YELLOW, KNnrow, KNncol, NONE);

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

	if(DEBUG_THREAD)
		printf("%sentrato in threadPool - initPool%s\n", YELLOW, NONE);

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
	tp->close = 0;

	loadFlagMap(tp->wator->plan, tp->flagMap);
	loadKNM(tp->KNM);

	/*
		inizializzo mutex e cw
	*/
	ec_not0(pthread_mutex_init(&(tp->queueLock), NULL), "errore initpool - inizializzazione lock");

	ec_not0(pthread_mutex_init(&(tp->KNMLock), NULL), "errore initpool - inizializzazione lock");

	ec_not0(pthread_cond_init(&(tp->waitingDispatcher), NULL), "errore initpool - inizializzazione cw waitingDispatcher");

	ec_not0(pthread_cond_init(&(tp->waitingWorkers), NULL), "errore initpool - inizializzazione cw waitingWorkers");

	ec_not0(pthread_cond_init(&(tp->waitingCollector), NULL), "errore initpool - inizializzazione cw waitingCollector");

	ec_not0(pthread_cond_init(&(tp->waitingTask), NULL), "errore initpool - inizializzazione cw waitingTask");

	/*
		inizializzo dispatcher
	*/
	ec_not0(pthread_create(&(tp->dispatcher), NULL, dispatcherTask, (void*) &tp), "errore initpool - creazione dispatcher");

	/*
		inizializzo collector
	*/
	ec_not0(pthread_create(&(tp->collector), NULL, collectorTask, (void*) &tp), "errore initpool - creazione collector");

	/*
		inizializzo i worker
	*/
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

		ec_not0(pthread_create(&(tp->workers[i]), NULL, workerTask, (void*) wa), "errore initpool - creazione i-esimo worker thread");
	}

	/*
		inizializzo gestore dei segnali
	*/
	sigset_t set;
	struct sigaction usr1;
	struct sigaction sint;
	struct sigaction term;
	struct sigaction salarm;
	struct sigaction pipe;

	ec_meno1(sigfillset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

	bzero(&usr1, sizeof(usr1));
	bzero(&sint, sizeof(sint));
	bzero(&term, sizeof(term));
	bzero(&salarm, sizeof(salarm));
	bzero(&pipe, sizeof(pipe));

	usr1.sa_handler = set_alarm;
	sint.sa_handler = set_close;
	term.sa_handler = set_close;
	salarm.sa_handler = set_check;
	pipe.sa_handler = SIG_IGN;

	ec_meno1(sigaction(SIGUSR1, &usr1, NULL));
	ec_meno1(sigaction(SIGINT, &sint, NULL));
	ec_meno1(sigaction(SIGTERM, &term, NULL));
	ec_meno1(sigaction(SIGALRM, &salarm, NULL));
	ec_meno1(sigaction(SIGPIPE, &pipe, NULL));

	pthread_create(&(tp->signal_handler), NULL, signalTask, (void*)  &tp);

	ec_meno1(sigemptyset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

	return 1;
}

void freePool(threadPool tp)
{
	int i = 0;

	if(DEBUG_THREAD)
		printf("%sentrato in threadPool - freePool%s\n", YELLOW, NONE);

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
	ec_not0(pthread_mutex_destroy(&(tp->queueLock)), "freePool destroy: errore queueLock");
	ec_not0(pthread_mutex_destroy(&(tp->KNMLock)), "freePool destroy: errore KNMLock");
	ec_not0(pthread_cond_destroy(&(tp->waitingCollector)), "freePool destroy: errore waitingCollector");
	ec_not0(pthread_cond_destroy(&(tp->waitingDispatcher)), "freePool destroy: errore waitingDispatcher");
	ec_not0(pthread_cond_destroy(&(tp->waitingWorkers)), "freePool destroy: errore waitingWorkers");
	ec_not0(pthread_cond_destroy(&(tp->waitingTask)), "freePool destroy: errore waitingTask");
}

int makeJoin(threadPool tp)
{
	int i = 0;
	/*
		faccio le join su tutti i thread <devo inserire il flag di chiusura>
	*/

	ec_not0(pthread_join(tp->signal_handler, NULL), "makeJoin join: errore signal_handler");

	if(DEBUG_THREAD)
		printf("%sSIGNAL_HANDLER terminato%s\n", YELLOW, NONE);

	ec_not0(pthread_join(tp->dispatcher, NULL), "makeJoin join: errore dispatcher");

	if(DEBUG_THREAD)
		printf("%sDISPATCHER terminato%s\n", YELLOW, NONE);

	ec_not0(pthread_join(tp->collector, NULL), "makeJoin join: errore collector");
	
	if(DEBUG_THREAD)
		printf("%sCOLLECTOR terminato%s\n", YELLOW, NONE);

	for(i = 0; i < tp->wator->nwork; i++)
	{
		ec_not0(pthread_join(tp->workers[i], NULL), "makeJoin join: errore workers");

		if(DEBUG_THREAD)
			printf("%sWORKER %d terminato%s\n", YELLOW, i, NONE);
	}	

	return 1;
}
