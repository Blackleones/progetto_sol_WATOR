/*
	\file wator_planet.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore
	ad esclusione della tecnica di "scelta randomizzata delle celle" della funzione getCellPositionWith.
	(verificabile tramite git log dell'autore).
*/

#include "wator.h"

/*cerco la cella con carattere 'c' e mano a mano che ne trovo calcolo
 la probabilità di sceglierla

	-->> 				--	IMPORTANTE	--						<<--

			l'algoritmo di scelta randomizzata è copiato da 
			"programmazione nella pratica" pagina 79 

	-->>				--	IMPORTANTE	--						<<--

	inizializzazione:
		*k = -1 *l = -1 questa inizializzazione ci è utile per gestire il caso in cui
		non troviamo alcuna cella adiacente che contiene l'animale 'c'
	
	l'algoritmo analizza le celle adiacenti servendosi della funzione mod() per gestire
	i casi speciali (x sul bordo | y sul bordo).
	se la cella in esame è occupata dall' animale 'c' allora vi è una probabila 1/nmatch di 
	scegliere quella cella.

	\retval *k *l
		se *k == -1 && *l == -1 implica che non esiste una cella adiacente a x,y che contiene 
		l'animale 'c'

		altrimenti ritorna la cella scelta
 */
static void getCellPositionWith(planet_t* planet, const int x, const int y, int* k, int* l, const char c)
{
	unsigned int nrow = planet->nrow;
	unsigned int ncol = planet->ncol;
	unsigned int nmatch = 0;
	cell_t** map = planet->w;
	
	*k = -1;/*se fuori da getCellWith k == -1 e l == -1 => cella non trovata*/
	*l = -1; 

	if(cell_to_char(map[mod(x-1, nrow)][y]) == c)
		if(rand() % ++nmatch == 0){/*cambiare: rand_r*/
			*k = mod(x-1, nrow);
			*l = y;
		}
	
	if(cell_to_char(map[mod(x+1, nrow)][y]) == c)
		if(rand() % ++nmatch == 0){
			*k = mod(x+1, nrow);
			*l = y;
		}
		
	if(cell_to_char(map[x][mod(y-1, ncol)]) == c)
		if(rand() % ++nmatch == 0){
			*k = x;
			*l = mod(y-1, ncol);
		}

	if(cell_to_char(map[x][mod(y+1, ncol)]) == c)
		if(rand() % ++nmatch == 0){
			*k = x;
			*l = mod(y+1, ncol);
		}
}

static int count(cell_t** map,unsigned const int nrow, unsigned const int ncol, const char animal)
{
	int counter = 0;
	int row_index = 0;
	int column_index = 0;

	for(row_index = 0; row_index < nrow; row_index++)
		for(column_index = 0; column_index < ncol; column_index++)
			if(cell_to_char(map[row_index][column_index]) == animal)
				counter++;

	return counter;
}

/*
	\param x,y posizione corrente
	\param k,l posizione prossima
	\c animale che deve essere aggiornato
	\action azione eseguita

	updateMat si occupa di aggiornare map, btime, dtime a seconda dell'azione 
	dall'animale 'c'
*/
static int updateMat(planet_t* planet, const int x, const int y, int k, int l, const char c, const int action)
{
	cell_t** map = planet->w;
	int** btime = planet->btime;
	int** dtime = planet->dtime;

	if(action == EAT || action == MOVE){
		map[k][l] = char_to_cell(c);
		map[x][y] = WATER;
		btime[k][l] = btime[x][y];
		btime[x][y] = 0;
		dtime[k][l] = dtime[x][y];
		dtime[x][y] = 0;

		return 0;
	}

	if(action == DEAD){
		map[x][y] = WATER;
		btime[x][y] = 0;
		dtime[x][y] = 0;

		return 0;		
	}

	errno = EINVAL;
	perror("UpdateMat");
	return -1;
}

/*
	\param x,y posizione attuale
	\param who animale da spostare

	la funzione controlla se i parametri x,y sono esatti e se l'animale che è
	contenuto in posizione x,y è davvero "who"

*/
static int check(planet_t* planet, const int x, const int y, const char who)
{
	cell_t** map = planet->w;

	if((x >= 0 && x < planet->nrow) || (y >= 0 && y < planet->ncol))
		if(cell_to_char(map[x][y]) != who)
			return 1;

	return 0;
}

/*
	\param k,l la posizione dove viene inserito il nuovo arrivato
	\param who il tipo del nuovo arrivato (shark/fish)

	inserisce il nuovo arrivato in posizione
*/
static void bornAt(planet_t* planet, int k, int l, const char who)
{
	cell_t** map = planet->w;
	int** btime = planet->btime;
	int** dtime = planet->dtime;

	map[k][l] = char_to_cell(who);
	btime[k][l] = 0;
	dtime[k][l] = 0;
}

/*
creo e inizializzo la mappa FLAG per assicurare che ogni pesce/squalo venga spostato
esattamente 1 volta per turno.

se leggo SHARK/FISH inizializzo la loro posizione con MOVE (da spostare) altrimenti
STOP (da non considerare)
*/

static int** loadFlagMap(planet_t* planet)
{
	int row_index = 0;
	int column_index = 0;
	cell_t** map = planet->w;
	int** flagMap = (int**) calloc(planet->nrow, sizeof(int*));

	if(flagMap == NULL)
		return NULL;

	for(row_index = 0; row_index < planet->nrow; row_index++){
		flagMap[row_index] = (int*) calloc(planet->ncol, sizeof(int));

		if(flagMap[row_index] == NULL)
			return NULL;
	}

	/*inizializzo i flag*/
	for(row_index = 0; row_index < planet->nrow; row_index++)
		for(column_index = 0; column_index < planet->ncol; column_index++)
			if(map[row_index][column_index] != WATER)
				flagMap[row_index][column_index] = MOVE;

	return flagMap;
}

int fish_count (planet_t* p)
{	
	return count(p->w, p->nrow, p->ncol, 'F');
}

int shark_count (planet_t* p)
{
	return count(p->w, p->nrow, p->ncol, 'S');
}

/*
	per tutte le regole:
	il controllo su k != -1 && l != -1 viene fatto per evitare un segmentation fault.
	ricevere da getCellPositionWith k == -1 e l == -1 vuol dire che non abbiamo trovato
	nessuna cella adiacente a x,y (corrente) che contenga l'animale specificato nell'
	ultimo parametro.
*/
int shark_rule1 (wator_t* pw, int x, int y, int *k, int* l)
{
	if(pw == NULL || k == NULL || l == NULL || check(pw->plan, x, y, cell_to_char(SHARK))){
		errno = EINVAL;
		perror("shark_rule1: ");
		return -1;
	}

	getCellPositionWith(pw->plan, x, y, k, l, cell_to_char(FISH));

	if(*k != -1 && *l != -1){
		if(updateMat(pw->plan, x, y, *k, *l, cell_to_char(SHARK), EAT) == -1)
			return -1;

		return EAT;
	}

	getCellPositionWith(pw->plan, x, y, k, l, cell_to_char(WATER));

	if(*k != -1 && *l != -1){
		if(updateMat(pw->plan, x, y, *k, *l, cell_to_char(SHARK), MOVE) == -1)
			return -1;	
		else
			return MOVE;
	}
	
	return STOP;
}

int shark_rule2 (wator_t* pw, int x, int y, int *k, int* l)
{
	planet_t* planet = pw->plan;
	int** btime = NULL;
	int** dtime = NULL;

	if(pw == NULL || k == NULL || l == NULL || check(pw->plan, x, y, cell_to_char(SHARK))){
		errno = EINVAL;
		perror("shark_rule2: ");
		return -1;
	}

	btime = planet->btime;
	dtime = planet->dtime;

	/*puo' partorire?*/
	if(btime[x][y] < pw->sb)
		btime[x][y]++;
	else
	{
		btime[x][y] = 0;
		getCellPositionWith(planet, x, y, k, l, cell_to_char(WATER));

		/*c'e' posto per lui?*/
		if(*k != -1 && *l != -1)
			bornAt(planet, *k, *l, cell_to_char(SHARK));
	}

	/*muore?*/
	if(dtime[x][y] < pw->sd){
		dtime[x][y]++;/*non devo modificare niente nella map*/
		return ALIVE;
	}
	else
	{
		dtime[x][y] = 0;
		if(updateMat(planet, x, y, *k, *l, cell_to_char(SHARK), DEAD) == -1)
			return -1;

		return DEAD;
	}

	/*se non sopravvivo ne muoio allora errore*/
	errno =  EWOULDBLOCK;
	perror("shark_rule2");
	return -1;
}

int fish_rule3 (wator_t* pw, int x, int y, int *k, int* l)
{	
	if(pw == NULL || k == NULL || l == NULL || check(pw->plan, x, y, cell_to_char(FISH))){
		errno = EINVAL;
		perror("fish_rule3: ");
		return -1;
	}	

	getCellPositionWith(pw->plan, x, y, k, l, cell_to_char(WATER));

	/*il pesce puo' spostarsi?*/
	if(*k != -1 && *l != -1){
		if(updateMat(pw->plan, x, y, *k, *l, cell_to_char(FISH), MOVE) == -1)
			return -1;	
		else
			return MOVE;
	}

	/*non sono riuscito a spostarmi*/
	return STOP;	
}

int fish_rule4 (wator_t* pw, int x, int y, int *k, int* l)
{	
	planet_t* planet = pw->plan;
	int** btime = NULL;

	if(pw == NULL || k == NULL || l == NULL || check(pw->plan, x, y, cell_to_char(FISH))){
		errno = EINVAL;
		perror("fish_rule4: ");
		return -1;
	}

	btime = planet->btime;

	/*puo' partorire?*/
	if(btime[x][y] < pw->fb)
		btime[x][y]++;
	else
	{
		btime[x][y] = 0;
		getCellPositionWith(planet, x, y, k, l, cell_to_char(WATER));

		/*c'e' posto per lui?*/
		if(*k != -1 && *l != -1)
			bornAt(planet, *k, *l, cell_to_char(FISH));
	}

	return 0;	
}

/*
	la funzione controlla se pw è inizializzato (!= NULL), crea una flagMap ed inizia
	la simulazione di 1 chronon.

	l'algoritmo si basa sulla costruzione della matrice flagMap:
		si scorre la matrice e ogni volta che leggiamo MOVE attiviamo le regole relative
		all'animale di posizione map[row_index][column_index]

		i casi speciali in cui shark/fish cambiano posizione vengono controllati. Ogni volta
		elaboriamo lo stato di uno shark/fish ci assicuriamo che non venga rivalutato piu' di
		una volta 
			caso EAT MOVE:
				si mette a STOP i flag k,l e x,y
			caso DEAD:
				si mette a stop i flag x,y
			caso STOP:
				si mette a stop i flag x,y
*/
int update_wator (wator_t * pw){
	int nrow = 0;
	int ncol = 0;
	int k, l;
	int row_index = 0;
	int column_index = 0;
	int action = 0;
	int** flagMap = NULL;
	planet_t* planet = NULL;
	cell_t** map = NULL;

	if(pw == NULL){
		errno = EINVAL;
		perror("update_wator");
		return -1;
	}

	planet = pw->plan;
	map = planet->w;
	nrow = planet->nrow;
	ncol = planet->ncol;
	flagMap = loadFlagMap(planet);

	if(flagMap == NULL){
		return -1;
	}

	for(row_index = 0; row_index < nrow; row_index++)
		for(column_index = 0; column_index < ncol; column_index++){

			if(flagMap[row_index][column_index] == MOVE && map[row_index][column_index] == SHARK){
				action = shark_rule2(pw, row_index, column_index, &k, &l);

				if(action == -1)
					return -1;

				if(action != DEAD){
					action = shark_rule1(pw, row_index, column_index, &k, &l);

					if(action == -1)
						return -1;

					if(action == EAT || action == MOVE)
						flagMap[k][l] = STOP;
				}

				flagMap[row_index][column_index] = STOP;
			}

			if(flagMap[row_index][column_index] == MOVE && map[row_index][column_index] == FISH){
				action = fish_rule4(pw, row_index, column_index, &k, &l);

				if(action == -1)
					return -1;

				action = fish_rule3(pw, row_index, column_index, &k, &l);

				if(action == -1)
					return -1;

				if(action == MOVE)
					flagMap[k][l] = STOP;

				flagMap[row_index][column_index] = STOP;
			}

		}

	for(row_index = 0; row_index < nrow; row_index++)
		free(flagMap[row_index]);

	free(flagMap);
	return 0;
}