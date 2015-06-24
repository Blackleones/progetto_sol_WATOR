#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "macro.h"
#include "wator.h"

#define SOCK_NAME "./visual.sck"
#define UNIX_PATH_MAX 108
#define SOCKET_BUFFER_SIZE 201

volatile int run = 1;

static void close_program()
{
	printf("chiusura visualizer...\n");
	run = 0;
}

void signal_handler()
{
	sigset_t set;
	struct sigaction pipe;
	struct sigaction sint;
	struct sigaction term;

	ec_meno1(sigfillset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

	bzero(&pipe, sizeof(pipe));
	bzero(&sint, sizeof(sint));
	bzero(&term, sizeof(term));

	pipe.sa_handler = SIG_IGN;
	sint.sa_handler = close_program;
	term.sa_handler = close_program;

	ec_meno1(sigaction(SIGPIPE, &pipe, NULL));
	ec_meno1(sigaction(SIGINT, &sint, NULL));
	ec_meno1(sigaction(SIGTERM, &term, NULL));

	ec_meno1(sigemptyset(&set));
	ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL));

}

void check_socket()
{
	int checkError = 0;

	if((checkError = access(SOCK_NAME, F_OK)) != -1)
	{
		if(unlink(SOCK_NAME) == -1)
		{
			error(EBADFD, "errore visualizer - cancellazione della socket");
			exit(EXIT_FAILURE);
		}
	}	
}
static void stampa(char* map, int ncol)
{
	int i = 0;

	usleep(100000);

	for(i = 0; i < ncol + 1; i++)
	{
		if(i != 0 && i != ncol)
		{
			if(map[i] == 'W')
				printf(" %s%c%s", BLUE, map[i], NONE);

			if(map[i] == 'S')
				printf(" %s%c%s", RED, map[i], NONE);

			if(map[i] == 'F')
				printf(" %s%c%s", GREEN, map[i], NONE);
		}
		else
		{
			if(map[i] == 'W')
				printf("%s%c%s", BLUE, map[i], NONE);

			if(map[i] == 'S')
				printf("%s%c%s", RED, map[i], NONE);

			if(map[i] == 'F')
				printf("%s%c%s", GREEN, map[i], NONE);	
		}
	}
	
	printf("\n");
}

int main(int argc, char* argv[])
{
	int nrow = 0;
	int ncol = 0;

	char buffer[SOCKET_BUFFER_SIZE] = "\0";

	int fd_socket = -1;
	int fd_client = -1;
	struct sockaddr_un socketAddress;

	signal_handler();
	check_socket();

	if(argc != 3)
	{
		error(EINVAL, "errore visualizer - inserire nrow ncol");
		exit(EXIT_FAILURE);
	}

	if(isNum(argv[1]) == -1 || isNum(argv[2]) == -1)
	{
		error(EINVAL, "errore visualizer - nrow e/o ncol non sono interi");
		exit(EXIT_FAILURE);
	} 
	
	nrow = atoi(argv[1]);
	ncol = atoi(argv[2]);

	strncpy(socketAddress.sun_path, SOCK_NAME, UNIX_PATH_MAX);
	socketAddress.sun_family = AF_UNIX;
	fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	if(bind(fd_socket, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) == -1)
	{
		perror("errore visualizer - bind");
		exit(EXIT_FAILURE);
	}

	if(listen(fd_socket, SOMAXCONN) == -1)
	{
		perror("errore visualizer - listen");
		exit(EXIT_FAILURE);
	}

	while(run)
	{
		if((fd_client = accept(fd_socket, NULL, 0)) == -1)
		{
			if(errno == EINTR)
				break;

			perror("errore visualizer - accept");
			exit(EXIT_FAILURE);
		}

		printf("%d\n%d\n", nrow, ncol);

		while(read(fd_client, buffer, ncol) > 0)
		{
			stampa(buffer, ncol);

			if(write(fd_client, "ok\0", strlen("ok\0")) == -1)
			{
				perror("errore visualizer - risposta al client");
				exit(EXIT_FAILURE);
			}

		}

		printf("\n");
		close(fd_client);
	}

	close(fd_socket);
	exit(EXIT_SUCCESS);
}