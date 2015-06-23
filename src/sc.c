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

int main()
{
	char buffer[11] = "WWWFFSSWWF\0";
	char result[3] = "\0";
	int i = 0;
	int fd_socket;
	struct sockaddr_un socketAddress;

	strncpy(socketAddress.sun_path, SOCK_NAME, UNIX_PATH_MAX);
	socketAddress.sun_family = AF_UNIX;
	fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);


	while(connect(fd_socket, (struct sockaddr*) &(socketAddress), sizeof(socketAddress)) == -1)
	{
		if(errno == ENOENT)
			sleep(1); 
		else
			exit(EXIT_FAILURE);
	}

	for(i = 0; i < 10; i++)
	{	
		//printf("%s\n", buffer);
		if(write(fd_socket, buffer, strlen(buffer)) == -1)
		{
			perror("ERRORE");
			exit(EXIT_FAILURE);
		}

		//bzero(buffer, sizeof(buffer));

		if(read(fd_socket, result, SOCKET_BUFFER_SIZE) == -1)
		{
			perror("ERRORE");
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
}