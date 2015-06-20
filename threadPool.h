#ifndef _THREADPOOL_
#define _THREADPOOL_
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "macro.h"
#include "queue.h"
#include "wator.h"
#define K 2
#define N 2

#ifndef DEBUG_THREAD
#define DEBUG_THREAD 0
#endif

#ifndef DEBUG_THREAD_MATRIX
#define DEBUG_THREAD_MATRIX 0
#endif

#ifndef DEBUG_THREAD_TASK
#define DEBUG_THREAD_TASK 0
#endif

typedef enum _status {WAITING, RUNNING, DONE} status;

typedef struct __KNmatrix _KNmatrix;
typedef _KNmatrix* KNmatrix;

struct __KNmatrix
{
	int nrow;
	int ncol;
	status** matrix;
};

typedef struct __threadPool _threadPool;
typedef _threadPool* threadPool;

struct __threadPool
{
	/*
		coda dei task da elaborare
	*/
	myQueue taskqueue;
	/*
		matrice KN di supporto per elaborare il pianeta 
	*/
	KNmatrix KNM;
	/*
		matrice di supporto per controllare gli spostamenti degli animali
	*/
	int** flagMap;
	/*
		struttura dati del pianeta
	*/
	wator_t* wator;
	/*
		i thread
	*/
	pthread_t dispatcher;
	pthread_t collector;
	pthread_t* workers;

	/*
		lock da utilizzare sulla taskqueue
	*/
	pthread_mutex_t queueLock;
	/*
		lock da utilizzare sulla matrice KNM per la sincronizzazione dei thread
	*/
	pthread_mutex_t KNMLock;
	/*
		viene usata dai worker per aspettare che il dispatcher abbia riempito
		la taskqueue
	*/
	pthread_cond_t waitingDispatcher;
	/*
		viene usata dal dispatcher per aspettare che sia terminato 1 ciclo di
		elaborazione del chronon
	*/
	pthread_cond_t waitingCollector;
	/*
		viene usata dal collector per aspettare che i worker abbiano "finito",
		quando un worker vede la lista vuota avvisa il collector
	*/
	pthread_cond_t waitingWorkers;
	/*
		viene usata dai worker per rimanere in attesa nel caso ci sia
		un thread che sta elaborando una porzione della mappa confinante
		alla sua
	*/
	pthread_cond_t waitingTask;
	/*
		# worker che stanno effettuando una elaborazione
	*/
	volatile int workingThread;
	/*
		flag per avviare il collector
	*/
	volatile int collectorFlag;
	/*
		flag per avviare i worker
	*/
	volatile int workFlag;
	/*
		flag per avviare tutti i thread
	*/
	volatile int run;
};

typedef struct __workerargs _workerargs;
typedef _workerargs* workerargs;

struct __workerargs
{
	int n;
	threadPool tp;
};

/*
	\param threadPool, la struttura dati principale da inizializzare

	initpool inizializza tutti i campi della struttura dati

	\retval 1 se l'inizializzazione della struttura dati è andata a buon fine
	\retval -1 se c'e' stato un errore, setta errno
*/
int initpool(threadPool, wator_t*);

/*
	\param threadPool, la struttura dati principale che stiamo inizializzando

	inizializza la matrice KNmatrix di supporto
	
	\retval status** la KNmatrix
	\retval NULL se c'e' stato un errore, setta errno
*/
KNmatrix initKNmatrix(planet_t* plan);
/*
	\param threadPool, la struttura dati principale da liberare

	libera gli elementi allocati della struttura dati
*/
void freePool(threadPool);

/*
	il thread worker verifica se è stato settato workFlag ed inizia la sua
	elaborazione:
		se la taskqueue è vuota si mette in attesa
		se la taskqueue contiene un elemento, lo estrae, lo elabora e se
		adesso la taskqueue è vuota setta collectorFlag e gli invia un segnale
*/
void* workerTask(void*);

/*
	il thread dispatcher verifica che la coda sia vuota,
		se lo è inizia a preparare i task, l'inserisce nella coda,
		setta workFlag = 1, sveglia i worker (broadcast) e si mette
		in attesa del collector
*/
void* dispatcherTask(void*);

/*
	il thread collector verifica che la coda sia vuota, che workingThread == 0 
	e che collectorTask sia == 1 
		in tal caso setta work = 0, collectorFlag = 0 stampa il pianeta, aggiorna il chronon, 
		sveglia il dispatcher e si rimette in attesa dei worker
*/
void* collectorTask(void*);

/*
	\param threadPool, struttura dati principale

	questa funzione crea le Join con tutti i thread della struttura dati principale
*/
int makeJoin(threadPool);

/*
	\param threadPool, la struttura dati principale

	questa funzione calcola le coordinate della sottomatrice relativa al task che stiamo creando.
*/
void populateQueue(threadPool);

/*
	\param KNmatrix, la matrice di supporto per i thread
	\param int i, int j, le coordinate del quadrante che stiamo cercando di elaborare

	la funzione verifica che i quadranti confinanti con KNmatrix[i][j] non stiano gia evolvendo

	\retval 1 se nessuno dei quadranti confinanti è in evoluzione (RUNNING)
	\retval 0 se uno dei quadranti confinanti è in evoluzione 
*/
int checkMutex(KNmatrix, int, int);
/*

*/
void loadKNM(KNmatrix);

/*
	\param planet_t*, pianeta che deve evolvere
	\param int**, matrice di supporto per gli spostamenti

	questa funzione prepara la flagMap mettendo tutti gli
	elementi != WATER uguali a MOVE

	MOVE: l'animale di posizione [x][y] puo' spostarsi
	STOP: l'animale di posizione [x][y] si è gia spostato / 
			in posizione [x][y] c'è l'acqua

	\retval 1 se la preparazione è andata a buon fine
	\retval -1 altrimenti
*/
void loadFlagMap(planet_t*, int**);

/*
	\param task, il task da eseguire
	\param planet_t*, il pianeta da evolvere
	\param int**, la flagMap di supporto per gli spostamenti

	la funzione evolve una porzione del pianeta identificata da
		startX, stopX
		startY, stopY

	e verificando se l'animale di posizione [x][y] puo' essere spostato o 

	\retval 1 se l'evoluzione è riuscita con successo
	\retval -1 altrimenti
*/
int evolve(task, wator_t*, int**);

#endif