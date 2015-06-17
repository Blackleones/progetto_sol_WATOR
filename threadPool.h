#ifndef _THREADPOOL_
#define _THREADPOOL_
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "macro.h"
#include "queue.h"
#include "wator.h"
#define K 3
#define N 3
typedef enum _status {ELABORATO, IN_ELABORAZIONE,  DA_ELABORARE} status;

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
	status** KNmatrix;
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
status** initKNmatrix(threadPool);
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
		in tal caso setta work = 0, collectorFlag = 0 stampa il pianeta, aggiorna il chronon, sveglia il dispatcher
		e si rimette in attesa dei worker
*/
void* collectorTask(void*);

int makeJoin(threadPool);

#endif