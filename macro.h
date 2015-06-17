/*
	\file wator_wator.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/
#ifndef _MACRO_
#define _MACRO_
#include <stdio.h>

/* 
		***********************************************************************
		***	___________________________GENERALI____________________________ ***
		***																  	***
 		***********************************************************************
*/
/*se compilo con -DDEBUG setto DEBUG = 1 e avvio tutte le stampe di debug*/

#define ec_meno1(s) \
        if ( (s) == -1 ) { perror("ERRORE"); exit(errno); }
        
#define error(errnoval, message) {errno = errnoval; perror(message);}
#endif