/*
	\file wator_util.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/

#include "threadPool.h"

/*
	FUNZIONI DI DEBUG
*/
static void stampa(planet_t* planet)
{
	int i = 0, j = 0; 
	int nrow = planet->nrow;
	int ncol = planet->ncol;
	cell_t** map = planet->w;

	usleep(100000);
	system("clear");
	printf("\n");

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

	printf("%s\n|||||== %s: FLAGMAP ==|||||\n", message, YELLOW);

	for(i = 0; i < nrow; i++)
	{
		for(j = 0; j < ncol; j++)
			printf("%d ", flagMap[i][j]);

		printf("\n%s", NONE);
	}	
}

static void printTask(task t)
{
	printf("%s\n=============================================\n", YELLOW);
	printf("=\t[%d][%d]:\n=\t\tV: %d -> %d\n=\t\tO: %d -> %d\n=", t->i, t->j, t->startY, t->stopY, t->startX, t->stopX);
	printf("\n=============================================\n%s", NONE);
}

static void printKNM(KNmatrix knm, char* message)
{
	int i = 0, j = 0;

	printf("\n=======================%s======================\n%s", message, YELLOW);
	printf("	KN MATRIX					\n");
	for(i = 0; i < knm->nrow; i++){
		for(j = 0; j < knm->ncol; j++)
			printf("%d ", knm->matrix[i][j]);

		printf("\n");
	}
	printf("\n=============================================\n%s", NONE);
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

	task t = NULL;

	while(i < KNrow)
	{
		crf = ((cri + K - 1 < nrow) ? cri + K - 1 : nrow -1);
		cci = 0;
		ccf = 0;
		while(j < KNcol)
		{
			ccf = ((ccf + N - 1 < ncol) ? ccf + N - 1 : ncol-1);
			
			t = (task) malloc(sizeof(_task));
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

		/*
			acquisisco la lock ed inserisco i task da elaborare
		*/
		ec_not0(pthread_mutex_lock(&(tp->queueLock)), "dispatcher lock: errore queueLock");
		
		while(tp->workFlag != 0 || isEmpty(tp->taskqueue))
			ec_not0(pthread_cond_wait(&(tp->waitingCollector), &(tp->queueLock)), "dispatcher wait: errore waitingCollector");

		/*inserisco tutti i task*/
		if(DEBUG_THREAD)
			printf("%sDISPATCHER inserisce i task nella coda%s\n", YELLOW, NONE);

		populateQueue(tp);

		tp->workFlag = 1;
		ec_not0(pthread_cond_broadcast(&(tp->waitingDispatcher)), "dispatcher broadcast: waitingDispatcher");
		ec_not0(pthread_mutex_unlock(&(tp->queueLock)), "dispatcher unlock: errore queueLock");
	}

	if(DEBUG_THREAD)
		printf("%sDISPATCHER in chiusura%s\n", YELLOW, NONE);

	ec_not0(pthread_cond_broadcast(&(tp->waitingDispatcher)), "dispatcher broadcast exit: errore waitingDispatcher");
	pthread_exit(EXIT_SUCCESS);
}

void* workerTask(void* _wa)
{
	workerargs wa = ((workerargs) _wa);
	int n = wa->n;
	threadPool tp = wa->tp;
	myQueue taskqueue = tp->taskqueue;
	int** flagMap = tp->flagMap;
	task t = NULL;
	FILE* fd = NULL;
	char* filename = (char*) malloc(STRING_SIZE*sizeof(char));
	char* snum = (char*) malloc(STRING_SIZE*sizeof(char));

	/*
		effettuo la lock per non provocare una race condition su fopen
		senza lock ottengo una segmentation fault per nwork molto elevato
	*/

	sprintf(snum, "%d", n);
	strcpy(filename, WATOR_FILE);
	strcat(filename, snum);
	
	if((fd = fopen(filename, "w")) == NULL)
	{
		perror("threadPool - worker, errore creazione file");
		pthread_exit((void*) -1);
	}

	fclose(fd);
	free(snum);
	free(filename);

	while(tp->run)
	{
		/*
			acquisisco la lock
				-se la coda è vuota estraggo un task
			rilascio la lock
		*/
		ec_not0(pthread_mutex_lock(&(tp->queueLock)), "worker lock: errore queueLock");

		while((tp->workFlag == 0 || !isEmpty(tp->taskqueue)) && tp->run == 1)
			ec_not0(pthread_cond_wait(&(tp->waitingDispatcher), &(tp->queueLock)), "worker wait: errore waitingDispatcher");

		if(tp->run == 0)
		{
			if(DEBUG_THREAD)
				printf("%sWORKER %d ha trovato tp->run == 0, si sta chiudendo\n%s", YELLOW, n, NONE);

			ec_not0(pthread_cond_broadcast(&(tp->waitingDispatcher)), "worker broadcast: errore waitingDispatcher");
			ec_not0(pthread_mutex_unlock(&(tp->queueLock)), "worker unlock: errore queueLock");
			break;
		}

		if(DEBUG_THREAD)
			printf("%sWORKER %d esegue una pop\n%s", YELLOW, n, NONE);

		t = pop(taskqueue);

		if(DEBUG_THREAD_TASK)
			printTask(t);

		ec_not0(pthread_mutex_unlock(&(tp->queueLock)), "worker unlock: errore queueLock");

		/*
			acquisisco la lock
				-mi metto in attesa se un thread sta elaborando una porzione vicino al mio task
			rilascio la lock
			elaboro
		*/
		ec_not0(pthread_mutex_lock(&(tp->KNMLock)), "worker lock: errore KNMLock");

		if(DEBUG_THREAD_MATRIX)
			printKNM(tp->KNM, "WORKER:");

		while(checkMutex(tp->KNM, t->i, t->j) == 0)
			ec_not0(pthread_cond_wait(&(tp->waitingTask), &(tp->KNMLock)), "worker wait: errore waitingTask");

		tp->KNM->matrix[t->i][t->j] = RUNNING;
		ec_not0(pthread_mutex_unlock(&(tp->KNMLock)), "worker unlock: errore KNMLock");

		evolve(t, tp->wator, flagMap);

		ec_not0(pthread_mutex_lock(&(tp->KNMLock)), "worker lock: errore KNMLock");
		tp->KNM->matrix[t->i][t->j] = DONE;		
		free(t);
		/*se la cosa è vuota sveglio il collector*/
		if(checkMutexDone(tp->KNM) == 1){
			tp->collectorFlag = 1;
			ec_not0(pthread_cond_signal(&(tp->waitingWorkers)), "worker signal: errore waitingWorkers");
		}

		/*sveglio tutti i worker in attesa per elaborare il task*/
		ec_not0(pthread_cond_broadcast(&(tp->waitingTask)), "worker broadcast: errore waitingTask");
		ec_not0(pthread_mutex_unlock(&(tp->KNMLock)), "worker unlock: errore KNMLock");
	}

	free(wa);
	ec_not0(pthread_cond_broadcast(&(tp->waitingDispatcher)), "worker broadcast exit: errore waitingDispatcher");
	pthread_exit(EXIT_SUCCESS);
}

void* collectorTask(void* _tp)
{
	threadPool tp = *((threadPool*) _tp);
	myQueue taskqueue = tp->taskqueue;
	volatile int currentChronon = 0;

	while(tp->run)
	{
		ec_not0(pthread_mutex_lock(&(tp->KNMLock)), "collector lock: errore KNMLock");

		if(DEBUG_THREAD_TASK)
			printKNM(tp->KNM, "COLLECTOR");

		if(DEBUG_THREAD)
			printf("%sCOLLECTOR elementi in coda = %d%s\n", YELLOW, taskqueue->size, NONE);
		
		while(tp->collectorFlag == 0)
			ec_not0(pthread_cond_wait(&(tp->waitingWorkers), &(tp->KNMLock)), "collector wait: errore waitingWorkers");

		tp->collectorFlag = 0;
		tp->workFlag = 0;

		if(DEBUG_THREAD_MATRIX)
		{
			printKNM(tp->KNM, "COLLECTOR:");
			printFlagMap(tp->flagMap, tp->wator->plan->nrow, tp->wator->plan->ncol, "COLLECTOR");
		}

		if(currentChronon % tp->wator->chronon == 0)
		{
			
			if(DEBUG_THREAD)
				stampa(tp->wator->plan);

			send_planet(tp->wator->plan);
			currentChronon = 0;

			if(tp->close)
				tp->run = 0;
		}
		else
			currentChronon++;

		loadFlagMap(tp->wator->plan, tp->flagMap);
		loadKNM(tp->KNM);

		ec_not0(pthread_cond_signal(&(tp->waitingCollector)), "collector signal: errore waitingCollector");
		ec_not0(pthread_mutex_unlock(&(tp->KNMLock)), "collector unlock: errore KNMLock");
	}

	pthread_exit(EXIT_SUCCESS);
}

static void prepareBuffer(planet_t* plan, char* buffer, int row)
{
	int i = 0;

	for(i = 0; i < plan->ncol; i++)
		buffer[i] = cell_to_char(plan->w[row][i]);

	buffer[i] = '\0';
}

void send_planet(planet_t* plan)
{
	int i = 0;
	char* buffer = (char*) malloc((plan->ncol+1)*sizeof(char));
	char result[3] = {'0', '0', '\0'};

	int fd_socket;
	struct sockaddr_un socketAddress;

	if(DEBUG_THREAD)
		printf("%sthreadPool - send_planet%s\n", YELLOW, NONE);

	strncpy(socketAddress.sun_path, SOCK_NAME, UNIX_PATH_MAX);
	socketAddress.sun_family = AF_UNIX;
	fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	while(connect(fd_socket, (struct sockaddr*) &(socketAddress), sizeof(socketAddress)) == -1)
	{
		if(errno == ENOENT)
			sleep(1); 
		else
		{
			perror("threadPool - errore send_planet - connect");
			exit(EXIT_FAILURE);
		}
	}

	for(i = 2; i < plan->nrow; i++)
	{
		prepareBuffer(plan, buffer, i);

		if(write(fd_socket, buffer, strlen(buffer)) == -1)
		{
			perror("errore threadPool - send_planet write");
			exit(EXIT_FAILURE);
		}

		if(read(fd_socket, result, SOCKET_BUFFER_SIZE) == -1)
		{
			perror("errore threadPool - send_planet read");
			exit(EXIT_FAILURE);
		}

		memset(buffer, 0, (plan->ncol*+1)*sizeof(char));
	}

	free(buffer);
	fflush(NULL);
	close(fd_socket);
}