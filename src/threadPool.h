/*
	\file wator_util.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/

#ifndef _THREADPOOL_
#define _THREADPOOL_
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "macro.h"
#include "queue.h"
#include "wator.h"

#define K 5
#define N 5

#define STRING_SIZE 101
#define WATOR_FILE "wator_worker_"
#define WATOR_CHECK "wator.check"
#define SECS 1

#define SOCK_NAME "./visual.sck"
#define UNIX_PATH_MAX 108
#define SOCKET_BUFFER_SIZE 201

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
	pthread_t signal_handler;
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
	/*
		flag per la chiusura gentile
	*/
	volatile int close;
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

	initpool inizializza tutti i campi della struttura dati e crea le maschere per catturare 
	i segnali.

	\retval 1 se l'inizializzazione della struttura dati è andata a buon fine
	\retval -1 se c'e' stato un errore, setta errno
*/
int initpool(threadPool, wator_t*);

/*
	\param threadPool, la struttura dati principale che stiamo inizializzando

	inizializza la matrice KNmatrix di supporto dei task. Tutti i quadranti
	vengono settati a WAITING
	
	\retval status** la KNmatrix
	\retval NULL se c'e' stato un errore, setta errno
*/
KNmatrix initKNmatrix(planet_t* plan);
/*
	\param threadPool, la struttura dati principale da liberare

	libera gli elementi allocati della struttura dati principale e richiama
	le free delle librerie d'appoggio.
*/
void freePool(threadPool);

/*
	il thread signalTask controlla se sono stati settati i seguenti flag:

	-flag_check: posso scrivere sul file wator.check
	-flag_create: posso creare / aprire il file wator.check
	-flag_close: devo chiudere gentilmente il programma
*/
void* signalTask(void*);
/*
	\param workerargs: struttura dati che contiene il wid del thread e la struttura principale
		threadPool per poter eseguire la simulazione.
	il thread worker verifica se è stato settato workFlag ed inizia la sua
	elaborazione:
		se la taskqueue è vuota si mette in attesa
		se la taskqueue contiene un elemento:
			-acquisisce la lock sulla coda
			-lo estrae
			-rilascia la lock sulla coda
			-acquisisce la lock sulla matrice di supporto dei task e se nessun quadrante
				è in elaborazione => setta a RUNNING il quadrante in elaborazione, 
				rilascia la lock sulla matrice di supporto, esegue la elaborazione, 
				altrimenti rimane in attesa.
			-una volta terminata la elaborazione, acquisisce la lock sulla matrice di supporto,
			 setta  DONE il quadrante elaborato.
			-rilascia la lock

			se la matrice di supporto ai task è completamente elaborata (== DONE) allora
			mando un segnale al thread collector

	prima di terminare invia un segnale broadcast a tutti i thread worker in attesa per poterli terminare
*/
void* workerTask(void*);

/*
	\param threadPool: struttura dati principale per poter eseguire la simulazione.

	il thread dispatcher verifica che la coda sia vuota,
		se lo è inizia a preparare i task, l'inserisce nella coda,
		setta workFlag = 1, sveglia i worker (broadcast) e si mette
		in attesa del collector

	alla chiusura invia un segnale broadcast a tutti i thread worker per poterli terminare
*/
void* dispatcherTask(void*);

/*
	il thread collector verifica che collectorTask sia == 1 
		in tal caso 
			setta work = 0, 
			collectorFlag = 0 
			stampa il pianeta se il chronon è quello richiesto,
			aggiorna il chronon,
			sveglia il dispatcher
			e si rimette in attesa dei worker.

	nota:	Dopo aver stampato controlla se il flag tp->close = 1, in tal caso mette tp->run = 0
			uccidendo tutti i thread in vita (dispatcher, worker e collector stesso)
*/
void* collectorTask(void*);

/*
	\param threadPool, struttura dati principale

	questa funzione crea le Join con tutti i thread della struttura dati principale

	\retval 1 se non ci sono stati errori.
*/
int makeJoin(threadPool);

/*
	\param threadPool, la struttura dati principale

	questa funzione calcola le K*N sottomatrici, crea i relativi task per l'elaborazione e l'
	inserisce nella coda
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
	\param KNMmatrix, la matride di supporto per i thread

	la funzione ritorna 1 se tutti i quadranti sono stati elaborati, 
	altrimenti 0

	\retval 1, tutti i quadranti == DONE
	\retval 0, altrimenti
*/
int checkMutexDone(KNmatrix);
/*
	\param KNmatrix, la matrice di supporto per i thread

	la funzione setta tutta la matrice a WAITING
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

	verificando se l'animale di posizione [x][y] puo' essere spostato o meno 
	tramite la matrice di supporto flagMap

	\retval 1 se l'evoluzione è riuscita con successo
	\retval -1 altrimenti
*/
int evolve(task, wator_t*, int**);

/*
	\param planet_t* plan: il pianeta da stampare
	
	la funzione esegue una connessione con la socket e inizia a inviare il
	pianeta da stampare.

	il pianeta viene passato 1 riga per volta, eliminando i <blank>	
*/
void send_planet(planet_t* plan);

#endif
