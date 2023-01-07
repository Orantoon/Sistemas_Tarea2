#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 4444

// Global Variables
char buffer[1024];

struct AcceptedSocket
{
	int acceptedSocketFD;
	struct sockaddr_in address;
	int error;
	bool acceptedSuccessfully;
	char username[256];
	int convPriv;	// socket del cliente con el que se hablara de forma privada
};

struct AcceptedSocket acceptedSockets[30];
int acceptedSocketsCount = 0;

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD);
void acceptNewClient(int serverSocketFD);
void clientThread(struct AcceptedSocket *pSocket);
void optionRecv(struct AcceptedSocket *pSocket);
void clientLeft(struct AcceptedSocket *pSocket);

void nuevaConv(struct AcceptedSocket *pSocket);
bool existeUser(char asking[256], char user[256]);
int getSocket(char asking[256], char user[256]);
void esperarUser(struct AcceptedSocket *pSocket);
void chatPriv(struct AcceptedSocket *pSocket);

void nuevoGrupo();
void cantUsuarios(int socketFD);
void listaUsuarios(int socketFD);

// recv(new_socket, buffer, 1024, 0);			RECEIVE
// send(new_socket, buffer, strlen(buffer), 0);	SEND

int main(int argc, char const *argv[])
{
	
	// create a socket
	int server_socket, ret;
	struct sockaddr_in server_address;

	struct sockaddr_in new_address;
	
	socklen_t addr_size;

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
	
	acceptNewClient(server_socket);
	
	printf("Nos vemos!\n");
	shutdown(server_socket, SHUT_RDWR);

	return 0;
}

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD){
	struct sockaddr_in clientAddress;
	socklen_t addr_size;
	int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddress, &addr_size);
	
	struct AcceptedSocket* acceptedSocket = malloc(sizeof(struct AcceptedSocket));
	acceptedSocket->address = clientAddress;
	acceptedSocket->acceptedSocketFD = clientSocketFD;
	acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;
	acceptedSocket->convPriv = 0;
	
	// Get username
	recv(clientSocketFD, buffer, 1024, 0);
	
	strcpy (acceptedSocket->username, buffer);
	
	printf("Hola %s!\n\n", buffer);
	bzero(buffer, sizeof(buffer));
	
	
	if (!acceptedSocket->acceptedSuccessfully)
		acceptedSocket->error = clientSocketFD;
		
	return acceptedSocket;
}

void acceptNewClient(int serverSocketFD){
	while(true)
	{
		struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
		acceptedSockets[acceptedSocketsCount++] = *clientSocket;
		
		clientThread(clientSocket);
	}
}

void clientThread(struct AcceptedSocket *pSocket){
	pthread_t id;
	pthread_create(&id, NULL, optionRecv, pSocket);
}

void optionRecv(struct AcceptedSocket *pSocket){
	bool cerrar = false;
	while (!cerrar){
		recv(pSocket->acceptedSocketFD, buffer, 1024, 0);
		

		int opcion = atoi(buffer);
		bzero(buffer, sizeof(buffer));
		
		switch(opcion){
			case 1:
				nuevaConv(pSocket);
				break;
			case 2:
				nuevoGrupo();
				break;
			case 3:
				cantUsuarios(pSocket->acceptedSocketFD);
				break;
			case 4:
				listaUsuarios(pSocket->acceptedSocketFD);
				break;
			case 5:
				cerrar = true;
		}
	}
	
	clientLeft(pSocket);	
}

void clientLeft(struct AcceptedSocket *pSocket){
	printf("El cliente %s se ha desconectado.", pSocket->username);
	for (int i = 0; i < acceptedSocketsCount; i++){
		if (acceptedSockets[i].acceptedSocketFD == pSocket->acceptedSocketFD){
			acceptedSockets[i] = acceptedSockets[i + 1];
		}
	}
	acceptedSocketsCount--;
	
	close(pSocket->acceptedSocketFD);
}


void nuevaConv(struct AcceptedSocket *pSocket){
	char user[256];

	while (true){
		recv(pSocket->acceptedSocketFD, buffer, 1024, 0);

		strcpy (user, buffer);
		bzero(buffer, sizeof(buffer));
		
		if (existeUser(pSocket->username, user)){
			strcpy (buffer, "Usuario Encontrado");
			send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
			break;
		} else {
			strcpy (buffer, "Usuario NO Encontrado");
			send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
		}
	}
	
	pSocket->convPriv = getSocket(pSocket->username, user);
	
	esperarUser(pSocket);
}

bool existeUser(char asking[256], char user[256]){
	for (int i = 0; i < 30; i++){
		if (strcmp(acceptedSockets[i].username, asking) != 0 && strcmp(acceptedSockets[i].username, user) == 0){
			return true;
		}
	}
	return false;
}

int getSocket(char asking[256], char user[256]){
	for (int i = 0; i < 30; i++){
		if (strcmp(acceptedSockets[i].username, asking) != 0 && strcmp(acceptedSockets[i].username, user) == 0){
			return acceptedSockets[i].acceptedSocketFD;
		}
	}
	return -1;
}

void esperarUser(struct AcceptedSocket *pSocket){
	bool salir = false;
	while (!salir){
		for (int i = 0; i < 30; i++){
			if (acceptedSockets[i].acceptedSocketFD == pSocket->convPriv){
				printf("%d\n\n",acceptedSockets[i].convPriv);
				if (acceptedSockets[i].convPriv == pSocket->acceptedSocketFD){
					printf("B\n\n");
					salir = true;
					break;
				}
			}
		}
	}
	
	strcpy (buffer, "Conectado");
	send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
	
	chatPriv(pSocket);
}

void chatPriv(struct AcceptedSocket *pSocket){
	while(true){
		recv(pSocket->acceptedSocketFD, buffer, 1024, 0);
		
		if (strcmp(buffer, "_EXIT_") == 0){
			send(pSocket->convPriv, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
			break;
		}
		
		send(pSocket->convPriv, buffer, strlen(buffer), 0);
		bzero(buffer, sizeof(buffer));
	}
}

void nuevoGrupo(){
	printf("Nuevo grupo\n");
}

void cantUsuarios(int socketFD){
	sprintf(buffer, "%d", acceptedSocketsCount);
	
	printf("Se encontraron %s usuarios conectados.\n\n", buffer);
	
	send(socketFD, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
}

void listaUsuarios(int socketFD){
	printf("Lista de usuarios conectados:\n\n");
	for (int i = 0; i < 30; i++){
		if (strcmp(acceptedSockets[i].username, "") == 0){
			continue;
		}
		strcat(buffer, "- ");
		strcat(buffer, acceptedSockets[i].username);
		strcat(buffer, "\n");
	}
	
	printf("%s\n\n", buffer);
	send(socketFD, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
}



