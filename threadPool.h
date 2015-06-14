#ifndef _THREADPOOL_
#define _THREADPOOL_
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "macro.h"
#include "queue.h"
#include "wator.h"

typedef struct __threadPool _threadPool;
typedef _threadPool* threadPool;

/*
	taskqueue: coda dei task da elaborare
	dispatcher, collector, workers: sono i thread 

	workingThread: variabile che conta il numero di 
		worker attivi in una elaborazione dei task.
		questo flag mi occorre per far partire il collector

	work: flag che avvia / ferma i worker.
		se work = 1 allora gli worker devono elaborare
		se work = 0 allora gli worker devono attendere
*/
struct __threadPool
{
	myQueue taskqueue;
	
	pthread_t dispatcher;
	pthread_t collector;
	pthread_t* workers;

	pthread_mutex_t queueLock;

	pthread_cond_t waitingDispatcher;
	pthread_cond_t waitingCollector;
	pthread_cond_t waitingWorkers;

	volatile int workingThread;
	volatile int work;
	volatile int run;
};

/*
	\param threadPool, la struttura dati principale da inizializzare

	initpool inizializza tutti i campi della struttura dati

	\retval 1 se l'inizializzazione della struttura dati è andata a buon fine
	\retval -1 se c'e' stato un errore, setta errno
*/
int initpool(threadPool);

void freePool(threadPool);

void* workerTask(void*);

/*
	il thread dispatcher verifica che la coda sia vuota,
		se lo è inizia a preparare i task, l'inserisce nella coda,
		setta work = 1, sveglia i worker (broadcast) e si mette
		in attesa del collector
*/
void* dispatcherTask(void*);

/*
	il thread collector verifica che la coda sia vuota e che workingThread == 0
		in tal caso setta work = 0, stampa il pianeta, aggiorna il chronon, sveglia il dispatcher
		e si rimette in attesa dei worker
*/
void* collectorTask(void*);

int makeJoin(threadPool);

#endif