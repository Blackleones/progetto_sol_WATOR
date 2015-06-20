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
/*stampa errori*/        
#define error(errnoval, message) {errno = errnoval; perror(message);}
        
/*colori console*/
#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE  "\x1B[34m"
#define NONE  "\033[0m"
#endif