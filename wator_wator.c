/*
	\file wator_wator.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/

#include "wator.h"
#define NROW_WATOR_CONF 3

/*
	il file wator.conf è un file con una struttura prestabilita
		x num
		y num
		z num
		
		dove le coppie <[xyz], num> non hanno posizione fissata.

	Per verificare che il file sia ben formatto dobbiamo:
		-verificare che si leggano esattamente 3 righe
		-verificare che contenga <blank><ritorno carrello> nelle posizioni corrette
		-verificare che contenga solamente i caratteri sd, sb, fb siano presenti esattamente 1
			volta

	l'algoritmo si basa sulla costruzione:
		andiamo a leggere ogni riga del file wator.conf e verifichiamo che i primi 2 caratteri
		che andiamo al leggere siano esattamente una delle coppie fra sd, sb, fb.
		I flag ci permettono di verificare che sd, sb, fb sono presenti 1 volta sola (se anche solo
		uno dei tre flag è settato a un valore diverso da 1 abbiamo un errore)
		la correttezza della formattazione viene controllata sapendo che all' indice 3 abbiamo un blank,
		mentre il ritorno carrello è controllato implicitamente dalla funzione isNum(). Infatti se 
		dopo l'indice 3 deve esserci solamente un numero.
*/
static int readWatorConf(wator_t* wator)
{
	int row_index = 0;
	int error = 0;
	int flagSD = 0, flagSB = 0, flagFB = 0; 
	FILE* confFile = fopen(CONFIGURATION_FILE, "r");
	char* input = (char*) malloc(RIGA_FILE_SIZE*sizeof(char));

	if(confFile == NULL){
		free(input);
		perror("readWatorConf: ");
		return -1;
	}

	if(input == NULL)
		return -1;

	while(fgets(input, RIGA_FILE_SIZE, confFile) != NULL && !error && row_index < NROW_WATOR_CONF){
		/*se dopo sd/sb/fb non trovo un blank e se dopo il blank non trovo un numero: errore*/
		if(input[2] != ' ' && isNum(input+2) == -1)/*isblank mi da errore*/
			error = 1;
		else if(input[0] == 's' && input[1] == 'd'){
			wator->sd = atoi(input+2);
			flagSD = 1;
		}
		else if(input[0] == 's' && input[1] == 'b'){
			wator->sb = atoi(input+2);
			flagSB = 1;
		}
		else if(input[0] == 'f' && input[1] == 'b'){
			wator->fb = atoi(input+2);
			flagFB = 1;
		}else
			error = 1;

		row_index++;
	}

	fclose(confFile);
	free(input);

	/*file formattato male, lettura non terminata*/
	if(row_index != NROW_WATOR_CONF || error == 1 || flagSD == 0 || flagSB == 0 || flagFB == 0){
		errno = ERANGE;
		perror("readWatorConf: ");
		return -1;
	}

	return 0;
}

wator_t* new_wator (char* fileplan)
{	
	int error = 0;
	wator_t* wator = (wator_t*) malloc(sizeof(wator_t));
	planet_t* planet = NULL;
	FILE* inputPlanet = NULL;
	srand(time(NULL));

	if(wator == NULL)
		return NULL;

	error = readWatorConf(wator);

	if(error == -1)
		return NULL;

	inputPlanet = fopen(fileplan, "r");

	if(inputPlanet == NULL){
		perror("new_wator: ");/*problemi con il file di lettura*/
		return NULL;
	}

	planet = load_planet(inputPlanet);
	fclose(inputPlanet);

	if(planet == NULL)
		return NULL;

	wator->plan = planet;
	wator->nf = fish_count(wator->plan);
	wator->ns = shark_count(wator->plan);
	wator->nwork = NWORK_DEF;
	wator->chronon = CHRON_DEF;

	return wator;
}

void free_wator(wator_t* pw)
{
	free_planet(pw->plan);
	free(pw);
}