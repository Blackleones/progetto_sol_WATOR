/*
	\file wator_wator.c
	\author Leonardo Lurci
	Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
	(verificabile tramite git log dell'autore).
*/
#ifndef _MACRO_
#define _MACRO_

/* 
		***********************************************************************
		***	___________________________GENERALI____________________________ ***
		***																  	***
 		***********************************************************************
*/

#define ec_meno1(s) \
        if ( (s) == -1 ) { perror("ERRORE"); exit(errno); }

/* 
		*******************************************************************
		***	________________________WATOR_____________________________	***
		***															  	***
 		*******************************************************************
*/

#define RIGA_FILE_SIZE 128
#define NWORK_DEF 2
#define CHRON_DEF 2
        
#endif