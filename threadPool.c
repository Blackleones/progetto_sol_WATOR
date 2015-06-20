#include "threadPool.h"

/*
	FUNZIONI DI DEBUG
*/
static void stampa(planet_t* planet)
{
	usleep(100000);
	system("clear");
	printf("\n");
	int i = 0, j = 0; 
	int nrow = planet->nrow;
	int ncol = planet->ncol;
	cell_t** map = planet->w;

	for(i = 0; i < nrow; i++)
	{
		for(j = 0; j < ncol; j++)
		{
			if(cell_to_char(map[i][j]) == 'W')
				printf("%s%c%s ", BLUE, cell_to_char(map[i][j]), NONE);

			if(cell_to_char(map[i][j]) == 'S')
				printf("%s%c%s ", RED, cell_to_char(map[i][j]), NONE);

			if(cell_to_char(map[i][j]) == 'F')
				printf("%s%c%s ", GREEN, cell_to_char(map[i][j]), NONE);
		}

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

static void printTask(task t)
{
	printf("\n=============================================\n");
	printf("=\t[%d][%d]:\n=\t\tV: %d -> %d\n=\t\tO: %d -> %d\n=", t->i, t->j, t->startY, t->stopY, t->startX, t->stopX);
	printf("\n=============================================\n");
}

static void printKNM(KNmatrix knm, char* message)
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

/*
	FUNZIONI DI LIBRERIA
*/
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

int checkMutexDone(KNmatrix knm)
{
	int i = 0, j = 0;

	while(i < knm->nrow)
	{
		while(j < knm->ncol)
		{
			if(knm->matrix[i][j] == DONE)
				j++;
			else
				return 0;
		}

		i++;
	}

	return 1;
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

				flagMap[i][j] = STOP;
			}
		}
	}

	return 1;
}

void* dispatcherTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);

	while(tp->run)
	{
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
		tp->KNM->matrix[t->i][t->j] = DONE;		
		(tp->workingThread)--;

		/*se la cosa è vuota sveglio il collector*/
		if(checkMutexDone(tp->KNM) == 1){
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

		pthread_mutex_lock(&(tp->KNMLock));

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
			pthread_cond_wait(&(tp->waitingWorkers), &(tp->KNMLock));

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

		loadFlagMap(tp->wator->plan, tp->flagMap);
		loadKNM(tp->KNM);

		pthread_cond_signal(&(tp->waitingCollector));
		pthread_mutex_unlock(&(tp->KNMLock));
	}

	pthread_exit(EXIT_SUCCESS);
}