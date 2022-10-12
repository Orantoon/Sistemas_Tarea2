#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

int main(int argc, char const *argv[])
{

	fd_set readfds;
	int list_socket[30], i;
	char usernames[30];
	
	// initialize list_socket[] to 0
	for (i = 0; i < 30; i++){
		list_socket[i] = 0;
	}
	for (i = 0; i < 30; i++){
		usernames[i] = 0;
	}

	// create the server socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	// define the server address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9001);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// bind the socket to our specified IP and port
	bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

	// listen to connections
	if (listen(server_socket, 3) < 0){
		perror(listen);
		exit(EXIT_FAILURE);
	}

	int max_sd;
	int sd;
	int client_socket;
	int addrlen = sizeof(server_address);
	char username_response[256];
	while(1){
		// clear socket set
		FD_ZERO(&readfds);

		FD_SET(server_socket, &readfds);
		max_sd = server_socket;

		for (i = 0; i < 30; i++){
			sd = list_socket[i];

			if (sd > 0)
				FD_SET(sd, &readfds);

			if (sd > max_sd)
				max_sd = sd;
		}

		if (FD_ISSET(server_socket, &readfds)){
			if ((client_socket = accept(server_socket, (struct sockaddr *) &server_address, (socklen_t*) &addrlen)) < 0){
				perror("accept");
				exit(EXIT_FAILURE);
			}

			for (i = 0; i < 30; i++){
				if (list_socket[i] == 0){
					list_socket[i] = client_socket;

					recv(client_socket, &username_response, sizeof(username_response), 0);
					usernames[i] = username_response;
					printf(username_response);
					break;
				}
			}
		}

		for (i = 0; i < 30; i++){
			sd = list_socket[i];

			if (FD_ISSET(sd, &readfds)){
				close(sd);
				list_socket[i] = 0;
			}
		}
	}
	
	// send message
	//send(client_socket, server_message, sizeof(server_message), 0);

	// close socket
	close(server_socket);

	return 0;
}