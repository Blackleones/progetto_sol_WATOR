/*
	\file wator_planet.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/

#include "wator.h"

/*
	\param nrow
	\param ncol

	la funzione inizializza e restituisce una matrice d'interi inizializzata a 0
*/
static int** initBDtime(unsigned const int nrow, unsigned const int ncol)
{
	int row_index = 0;
	int** bdtime = (int**) calloc(nrow, sizeof(int*));

	if(bdtime == NULL)
		return NULL;

	for(row_index = 0; row_index < nrow; row_index++){
		bdtime[row_index] = (int*) calloc(ncol, sizeof(int));

		if(bdtime[row_index] == NULL)
			return NULL;
	}

	return bdtime;
}

/*
	\param nrow
	\param ncol

	la funzione crea una matrice cell_t* e la inizializza a WATER
*/
static cell_t** initMap(unsigned const int nrow, unsigned const int ncol)
{
	int row_index = 0;
	int column_index = 0;
	cell_t** map = (cell_t**) malloc(nrow*sizeof(cell_t*));

	if(map == NULL)
		return NULL;

	for(row_index = 0; row_index < nrow; row_index++){
		map[row_index] = (cell_t*) malloc(ncol*sizeof(cell_t));

		if(map[row_index] == NULL)
			return NULL;

		for(column_index = 0; column_index < ncol; column_index++)
			map[row_index][column_index] = WATER;
	}

	return map;
}

/*
	\param map matrice da liberare
	\param nrow 

	la funzione libera la matrice
*/
static void freeMap(unsigned const int nrow, cell_t** map)
{
	int row_index = 0;

	for(row_index = 0; row_index < nrow; row_index++)
		free(map[row_index]);	
}

/*
	\param bdtime matrice da liberare
	\param nrow 

	la funzione libera la matrice
*/
static void freeBDtime(int** bdtime, unsigned const int nrow)
{
	int row_index = 0;

	for(row_index = 0; row_index < nrow; row_index++)
		free(bdtime[row_index]);
}

/*
	\param p pianeta da liberare.
	la funzione non fa altro che effettuare le free() sulle variabili contenute 
	nella struttura planet_t

	btime, dtime, w vengono liberate dalle relative funzioni: freeBDtime(), freeMap()
*/
void free_planet (planet_t* p)
{
	if(p->w != NULL)
		freeMap(p->nrow, p->w);

	if(p->btime != NULL)
		freeBDtime(p->btime, p->nrow);

	if(p->dtime != NULL)
	freeBDtime(p->dtime, p->nrow);

	free(p->w);
	free(p->btime);
	free(p->dtime);
	free(p);
}

/*
	\param planet da cui estrarre la mappa
	\param nrow
	\param ncol
	\param f file gia aperto e posizionato sulla riga 0  
	precondizione: f != NULL -> viene controllato dal chiamante

	esempio di una riga di f:
	colonna: 012345678
		     W F W F S

	notiamo che ogni colonna multipla di 2 contiene l'informazione relativa a chi occupa
	la posizione della cella e ogni colonna non multipla di 2 è un blank.

	l'algoritmo si basa sulla costruzione:
		tramite il ciclo for scorriamo la matrice:
		data una riga i, tramite la variabile string_index saltiamo di 2 in 2 per prelevare 
		l'informazione da inserire nella matrice e al tempo stesso controlliamo che il file 
		ben formattato (<blank>info<blank>info<blank>....<blank><info><ritorno carrello>)

		se notiamo che il file non è ben formattato settiamo error = 1 e gestiamo l'errore
		come richiesto
*/
static void load_map(planet_t* planet, unsigned int nrow, unsigned int ncol, FILE* f)
{
	int error = 0;
	int row_index = 0;
	int column_index = 0;
	int string_index = 0;
	int sizeRow = 2*ncol+1;
	cell_t** map = planet->w;
	char* input = (char*) malloc(sizeRow*sizeof(char));

	while(fgets(input, sizeRow, f) != NULL && !error){
		
		for(column_index = 0; column_index < ncol && !error; column_index++){
			map[row_index][column_index] = char_to_cell(input[string_index]);

			/*controllo che il file sia ben formattato: contiene blank e \n*/
			if(input[string_index+1] != ' ' && input[string_index+1] != '\n')
				error = 1;

			/*controllo che il valore inserito non sia diverso da WFS*/
			if(map[row_index][column_index] == -1)
				error = 1;

			string_index += 2;/*salto lo spazio vuoto*/
		}
		
		string_index = 0;
		row_index++;		
	}

	free(input);

	if(row_index != nrow || error != 0){
		map = NULL;
		errno = ERANGE;
		perror("load_map: ");
		return;
	}
}

/*
	la funzione prepara le variabili che andremo ad usare (pianeta, nrow, ncol...)
	quando andiamo a leggere i due numeri relativi a nrow, ncol, prima di chiamare
	la funzione atoi() viene chiamata la funzione isNum() <wator_util.c> per verificare
	che abbiamo letto un numero.
*/
planet_t* load_planet (FILE* f)
{
	char* s_nrow = (char*) malloc(RIGA_FILE_SIZE*sizeof(char));
	char* s_ncol = (char*) malloc(RIGA_FILE_SIZE*sizeof(char));
	planet_t* planet = NULL;

	if(f == NULL){
		errno = EINVAL;
		perror("load_planet:");/*file not found*/
		free(s_nrow);
		free(s_ncol);
		return NULL;
	}

	fgets(s_nrow, RIGA_FILE_SIZE, f);
	fgets(s_ncol, RIGA_FILE_SIZE, f);

	if(isNum(s_nrow) == -1 && isNum(s_ncol) == -1){
		errno = ERANGE;
		perror("load_planet: load row/column number.");
		free(s_nrow);
		free(s_ncol);
		return NULL;
	}

	planet = new_planet(atoi(s_nrow), atoi(s_ncol));
	free(s_nrow);
	free(s_ncol);

	if(planet == NULL){
		return NULL;
	}

	load_map(planet, planet->nrow, planet->ncol, f);

	if(planet->w == NULL){/*lo posso anche togliere, il controllo è in load_map*/
		perror("load_planet");
		errno = ERANGE;
		return NULL;
	}

	return planet;
}

planet_t * new_planet (unsigned const int nrow, unsigned const int ncol)
{	
	planet_t* planet = (planet_t*) malloc(sizeof(planet_t));
	cell_t** map = NULL;
	int** btime = NULL;
	int** dtime = NULL;

	if(planet == NULL)
		return NULL;

	/*verifico che gli argomenti siano corretti*/
	if(nrow <= 0 || ncol <= 0){
		errno = EINVAL;
		free(planet);
		perror("initMap: ");
		return NULL;
	}

	planet->nrow = nrow;
	planet->ncol = ncol;
	planet->w = NULL;
	planet->btime = NULL;
	planet->dtime = NULL;
	map = initMap(nrow, ncol);
	btime = initBDtime(nrow, ncol);
	dtime = initBDtime(nrow, ncol);

	if(map == NULL || btime == NULL || dtime == NULL){
		free_planet(planet);
		return NULL;
	}

	planet->btime = btime;
	planet->dtime = dtime;
	planet->w = map;

	return planet;
}

int print_planet (FILE* f, planet_t* p)
{
	cell_t** map = p->w;
	int row_index = 0;
	int column_index = 0;

	if(f == NULL){
		errno = EINVAL;
		perror("print_planet: ");
		return -1;
	}

	fprintf(f, "%d\n%d\n", p->nrow, p->ncol);
	
	for(row_index = 0; row_index < p->nrow; row_index++){
		for(column_index = 0; column_index < p->ncol-1; column_index++)/*fino a ncol-1 perche l'ultimo è speciale*/
			fprintf(f, "%c ", cell_to_char(map[row_index][column_index]));

		/*l'ultimo carattere lo stampo senza blank*/
		fprintf(f, "%c", cell_to_char(map[row_index][column_index]));
		fprintf(f, "\n");
	}

	fflush(f);
	return 0;
}

/*
	buffer_planet2 si occupa di riempire il buffer da inviare alla socket

	se current_row < 2 vuol dire che devo leggere le prime 2 righe (nrow, ncol)
	se current_row >= 2 devo caricare il buffer con la riga current_row

	nota: siccome la matrice relativa alla mappa inizia dalla posizione 0,0
		per poter usare la varibile current_row devo sottrarre offset di nrow, ncol,
		ovvero 2
*/
void buffer_planet2(planet_t* planet, char* buffer, int* current_row)
{
	int pos = 0;
	int column_index = 0;
	cell_t** map = planet->w;

	if(*current_row == 0)
	{
		pos += sprintf(buffer+pos, "%d", planet->nrow);
		buffer[pos++] = '\n';

		(*current_row)++;
		return;
	}

	if(*current_row == 1)
	{
		pos += sprintf(buffer+pos, "%d", planet->ncol);
		buffer[pos++] = '\n';

		(*current_row)++;
		return;		
	}

	/*current_row -2 = tolgo le prime 2 righe dal conteggio*/
	for(column_index = 0; column_index < planet->ncol; column_index++)
		buffer[pos++] = cell_to_char(map[*current_row-2][column_index]);

	if(*current_row-2 != planet->ncol)
	buffer[pos++] = '\n';
	(*current_row)++;
}
