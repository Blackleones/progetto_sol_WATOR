#include <stdio.h>
#include <stdlib.h>
/*
	per compilare:
	gcc g -o NK provaNK.c

	per eseguire:
	./MK
*/
int main()
{
	system("clear");

	/*
		nrow, ncol relative alla matrice principale		
	*/
	int nrow = 0, ncol = 0;
	/*
		k*n grandezza delle sottomatrici
	*/
	int K = 0, N = 0;
	/*
		grandezza sottomatrice che tiene conto dei
		blocchi k*n
	*/
	int KNrow = 0, KNcol = 0;

	int i = 0, j = 0;

	printf("inserisci nrow, ncol:\t");
	scanf("%d %d", &nrow, &ncol);
	printf("inserisci k, n:\t");
	scanf("%d %d", &K, &N);

	KNrow = nrow / K + ((nrow % K != 0) ? 1 : 0);
	KNcol = ncol / N + ((ncol % N != 0) ? 1 : 0);

	/*
		cri: riga iniziale corrente
		crf: riga finale corrente
	*/
	int cri = 0, crf = 0;
	/*
		cci: colonna iniziale corrente
		ccf: colonna finale corrente
	*/
	int cci = 0, ccf = 0;

	while(i < KNrow)
	{
		crf = ((cri + K - 1 < nrow) ? cri + K - 1 : nrow -1);
		cci = 0;
		ccf = 0;
		while(j < KNcol)
		{
			ccf = ((ccf + N - 1 < ncol) ? ccf + N - 1 : ncol-1);
			/*
				new task:
					startX 	=	cri
					stopX 	=	crf
					startY	=	cci
					stopY	= 	ccf
			*/
			printf("[%d][%d]:\n\tV: %d -> %d\n\tO: %d -> %d\n", i, j, cci, ccf, cri, crf);

			cci = ((ccf + 1 < ncol) ? ccf + 1 : ncol - 1);
			ccf++;
			j++;
		}

		cri = ((crf + 1 < nrow) ? crf + 1 : nrow - 1);
		j = 0;
		i++;
	}

	return 0;
}