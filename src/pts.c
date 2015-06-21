#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define ec_meno1(s) \
        if ( (s) == -1 ) { perror("ERRORE"); exit(errno); }


volatile int run = 1;

static void stampa()
{
	printf("ho ricevuto il segnale\n");
	run = 0;
}

void* task()
{
	while(run);

	pthread_exit(0);
}

int thread_signal_handler(void)
{
	sigset_t set;
	struct sigaction usr1;

	ec_meno1(sigfillset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

	bzero(&usr1, sizeof(usr1));

	usr1.sa_handler = stampa;

	ec_meno1(sigaction(SIGUSR1, &usr1, NULL));

	pthread_t t = NULL;
	pthread_create(&t, NULL, task, NULL);

	ec_meno1(sigemptyset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));
	printf("prima join\n");
	pthread_join(t, NULL);
}

int main()
{
	thread_signal_handler();
	printf("io sono main");
	exit(EXIT_SUCCESS);
}