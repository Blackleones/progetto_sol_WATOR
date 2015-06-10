/*
	\file wator_util.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/
#include "wator.h"

char cell_to_char(cell_t a)
{
	if(a == SHARK)
		return 'S';
	if(a == FISH)
		return 'F';
	if(a == WATER)
		return 'W';

	return '?';
}

int char_to_cell(char c)
{
	if(c == 'S')
		return 0;
	if(c == 'F')
		return 1;
	if(c == 'W')
		return 2;

	return -1;
}

/*
	\param s stringa da controllare

	la funzione scorre la stringa e controlla se ogni carattere che legge è 
	un numero.

	\retval 1 se l'analisi è andata a buon fine (s è un numero)
	\retval -1 altrimenti
*/
int isNum(char* s)
{
	if(*s == '\0')
		return -1;

	while(*s != '\0' && *s != '\n')
		if(isdigit(*s))
			s++;
		else 
			return -1;

	return 1;	
}

/*
	l'operatore a % b in C è l'operatore che restituisce il RESTO e non il 
	modulo di a rispetto a b.

	problema: a % b = -a con a < 0

	soluzione:

		se a < 0
			ritorna b - |a| % b  [sottraggo da b il resto della divisione fra |a| e b]
		altrimenti
			ritorno a % b

	esempio: -5 % 4 = 4 - |-5| % 4 = 4 - 1 = 3
			  5 % 4 =  1 corretto.
*/
int mod(int a, unsigned int b){
	int result = abs(a) % b;
	return a < 0 ? b - result : result;
}