#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "macro.h"
#include "threadPool.h"
#include "wator.h"

#define PARSERSTRING ":n:v:f:"
/*
	compilare:
	clear;gcc -g -Wall -pedantic -pthread -o w watorprocess.c queue.c threadPool.c threadPool_init.c wator_planet.c wator_util.c wator_animals.c wator_wator.c 
*/
int main(int argc, char* argv[])
{
	/*
		indice per il parser
	*/
	int option = 0;
	/*
		n_worker, n_chronon definiti dall'utente
	*/
	int n_worker = 0;
	int n_chronon = 0;
	/*
		array per controllare che gli argomenti passati siano unici
	*/
	int optionFlag[] = {0,0,0};
	/*
		filedump: file su cui deve stampare il visualizer
		fileplanet: pianeta da elaborare
	*/
	char* filedump = NULL;
	char* fileplanet = NULL;
	/*
		strutture dati principali del programma
	*/
	wator_t* wator = NULL;
	threadPool threadpool = NULL;

	/*
		parser degli argomenti
	*/
	while ((option = getopt(argc, argv, PARSERSTRING)) != -1)
	{
		switch(option)
		{
			case 'n':
			{
				if(isNum(optarg) == -1)
				{
					error(EINVAL, "watorprocess - errore argomento n_worker");
					exit(EXIT_FAILURE);
				}

				n_worker = atoi(optarg);
				optionFlag[0]++;
				break;
			}
			case 'v':
			{
				if(isNum(optarg) == -1)
				{
					error(EINVAL, "watorprocess - errore argomento n_chronon");
					exit(EXIT_FAILURE);
				}

				n_chronon = atoi(optarg);
				optionFlag[1]++;
				break;
			}
			case 'f':
			{
				filedump = optarg;
				optionFlag[2]++;
				break;
			}
			case ':':
			{
				error(EINVAL, "watorprocess - manca argomento");
				exit(EXIT_FAILURE);
				break;
			}
			case '?':
			{
				error(EINVAL, "watorprocess - argomento non accettato");
				exit(EXIT_FAILURE);
				break;
			}
		}
	}

	if(optionFlag[0] > 1 || optionFlag[1] > 1 || optionFlag[2] > 1)
	{
		error(EINVAL, "watorprocess - alcuni argomenti sono ripetuti");
		exit(EXIT_FAILURE);
	}

	if((argc - optind) == 1)
	{
		/*apro il file*/
		fileplanet = argv[optind];

		/*
			controllo che il file esista e possa essere letto
		*/

		if(access(fileplanet, F_OK | R_OK) == -1)
		{
			perror("watorprocess - il pianeta non esiste o non puo' essere letto");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		error(EINVAL, "manca il planet o ci sono troppi argomenti");
		exit(EXIT_FAILURE);
	}

	/*
		creo il wator, inserisco n_worker e n_chronon se sono stati passati dall'utente
	*/
	if((wator = new_wator(fileplanet)) == NULL)
	{
		exit(EXIT_FAILURE);
	}

	if(optionFlag[0] != 0)
		wator->nwork = n_worker;

	if(optionFlag[1] != 0)
		wator->chronon = n_chronon;

	threadpool = (threadPool) malloc(sizeof(_threadPool));
	if(initpool(threadpool, wator) == -1)
	{
		perror("watorprocess - errore initpool");
		/*da gestire*/
	}

	makeJoin(threadpool);
	freePool(threadpool);

	printf("bye!\n");
	exit(EXIT_SUCCESS);
}