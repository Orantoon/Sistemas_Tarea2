#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 4444

// Global Variables
char buffer[1024];

int new_socket;

char usernames[30][256];
int clientsConnected[30];
int clientCount = 0;

void safeUN();
void optionRecv();
void nuevaConv();
void nuevoGrupo();
void cantUsuarios();
void listaUsuarios();

// recv(new_socket, buffer, 1024, 0);			RECEIVE
// send(new_socket, buffer, strlen(buffer), 0);	SEND

int main(int argc, char const *argv[])
{
	
	// create a socket
	int server_socket, ret;
	struct sockaddr_in server_address;

	struct sockaddr_in new_address;
	
	socklen_t addr_size;
	
	pid_t childpid;
	
	for (int i = 0; i < 30; i++){
		strcpy(usernames[i], "");
		clientsConnected[i] = 0;
	}
	
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0){
		printf("Error in the connection.\n\n");
		exit(1);
	}
	
	printf("Server Socket created successfully!\n\n");

	// define the server address
	memset(&server_address, '\0', sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// bind the socket to our specified IP and port
	ret = bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
	if (ret < 0){
		printf("Error in binding.\n\n");
		exit(1);
	}
	
	printf("Binded to port %d.\n\n", PORT);

	// listen to connections
	if (listen(server_socket, 30) == 0){
		printf("Listening...\n\n");
	} else {
		printf("Error in binding.\n\n");
	}
	
	// ===============================================
	
	while (true){
	
		new_socket = accept(server_socket, (struct sockaddr*)&new_address, &addr_size);
		if (new_socket < 0){
			exit(1);
		}
		
		clientsConnected[clientCount] = new_socket;
		clientCount++;
		
		printf("Connection accepted from %s:%d.\n\n", inet_ntoa(new_address.sin_addr), ntohs(new_address.sin_port));
		
		// Accept usernames
		recv(new_socket, buffer, 1024, 0);
		safeUN();
		
		if ((childpid = fork()) == 0){
		
			close(server_socket);

			while (true){
			
				// Gets option from the client
				optionRecv();

			}
		}
	
	}
	
	// close socket
	printf("Nos vemos!\n");
	//close(new_socket);

	return 0;
}

void safeUN(){
	strcpy(usernames[clientCount], buffer);
	printf("Hello %s!\n\n", usernames[clientCount]);
	
	bzero(buffer, sizeof(buffer));
}

void optionRecv(){
	recv(new_socket, buffer, 1024, 0);

	int opcion = atoi(buffer);
	bzero(buffer, sizeof(buffer));
	
	switch(opcion){
		case 1:
			nuevaConv();
			break;
		case 2:
			nuevoGrupo();
			break;
		case 3:
			cantUsuarios();
			break;
		case 4:
			listaUsuarios();
	}
}

void nuevaConv(){
	printf("Nueva conversacion\n");
}

void nuevoGrupo(){
	printf("Nuevo grupo\n");
}

void cantUsuarios(){
	sprintf(buffer, "%d", clientCount);
	
	printf("Se encontraron %s usuarios conectados.\n\n", buffer);
	
	send(new_socket, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
}

void listaUsuarios(){
	printf("Lista de usuarios conectados:\n\n");
	for (int i = 0; i < 30; i++){
		if (strcmp(usernames[i], "") == 0){
			continue;
		}
		strcat(buffer, "- ");
		strcat(buffer, usernames[i]);
		strcat(buffer, "\n");
	}
	
	printf("%s\n\n", buffer);
	send(new_socket, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
}



