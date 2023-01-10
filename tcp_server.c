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
#include <errno.h>

#define PORT 4444

// Global Variables
char buffer[1024];

// Estructura del cliente
struct AcceptedSocket
{
	int acceptedSocketFD;
	struct sockaddr_in address;
	int error;
	bool acceptedSuccessfully;
	char username[256];
	int convPriv;	// socket del cliente con el que se hablara de forma privada
	int convGrup[10];	// sockets de clientes a los que se les hablara de forma grupal
};

FILE *serverLog;

// Clientes conectados
struct AcceptedSocket acceptedSockets[30];
int acceptedSocketsCount = 0;

// Proceso de aceptar un nuevo cliente
struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD);
void acceptNewClient(int serverSocketFD);
void clientThread(struct AcceptedSocket *pSocket);
void optionRecv(struct AcceptedSocket *pSocket);
void clientLeft(struct AcceptedSocket *pSocket);

// Opcion de conversacion privada entre clientes
void nuevaConv(struct AcceptedSocket *pSocket);
bool existeUser(char username[256], char user[256]);
int getSocket(char asking[256], char user[256]);
void chatPriv(struct AcceptedSocket *pSocket);

// Opcion de conversacion grupal entre clientes
void nuevoGrupo(struct AcceptedSocket *pSocket);
void chatGrup(struct AcceptedSocket *pSocket);

// Otras consultas de los clientes
void cantUsuarios(struct AcceptedSocket *pSocket);
void listaUsuarios(struct AcceptedSocket *pSocket);

// recv(new_socket, buffer, 1024, 0);			RECEIVE
// send(new_socket, buffer, strlen(buffer), 0);	SEND


int main(int argc, char const *argv[])
{
	// ---------- Configuracion de Sockets -----------
	
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

	int yes = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
		perror("setsockopt");
		exit(1);
	}	
	
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
	
	// -------------------------------------------
	
	acceptNewClient(server_socket);
	
	printf("Nos vemos!\n");
	
	shutdown(server_socket, SHUT_RDWR);

	return 0;
}

// ===============================================

struct AcceptedSocket* acceptIncomingConnection(int serverSocketFD){
	struct sockaddr_in clientAddress;
	socklen_t addr_size;
	int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddress, &addr_size);
	
	struct AcceptedSocket* acceptedSocket = malloc(sizeof(struct AcceptedSocket));
	acceptedSocket->address = clientAddress;
	acceptedSocket->acceptedSocketFD = clientSocketFD;
	acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;
	acceptedSocket->convPriv = 0;
	
	for (int i = 0; i < 10; i++)
		acceptedSocket->convGrup[i] = 0;
	
	// Get username
	ssize_t size;
	if ((size = recv(clientSocketFD, buffer, 1024, 0)) == -1)
		fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
	
	strcpy (acceptedSocket->username, buffer);
	
	printf("Hola %s!\n\n", buffer);
	
	serverLog = fopen("serverLog.txt", "a");
	fprintf(serverLog, "%s %s %s", "Se ha conectado el cliente ", buffer, "\n");
	fclose(serverLog);
	
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
				nuevoGrupo(pSocket);
				break;
			case 3:
				cantUsuarios(pSocket);
				break;
			case 4:
				listaUsuarios(pSocket);
				break;
			case 5:
				cerrar = true;
		}
	}
	
	clientLeft(pSocket);	
}

void clientLeft(struct AcceptedSocket *pSocket){
	printf("El cliente %s se ha desconectado.\n\n", pSocket->username);
	
	serverLog = fopen("serverLog.txt", "a");
	fprintf(serverLog, "%s %s %s", "Se ha desconectado el cliente ", pSocket->username, "\n");
	fclose(serverLog);
	
	for (int i = 0; i < acceptedSocketsCount; i++){
		if (acceptedSockets[i].acceptedSocketFD == pSocket->acceptedSocketFD){
			acceptedSockets[i] = acceptedSockets[i + 1];
		}
	}
	acceptedSocketsCount--;
	
	close(pSocket->acceptedSocketFD);
}

// ===============================================

void nuevaConv(struct AcceptedSocket *pSocket){
	char user[256];
	pSocket->convPriv = 0;

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
	
	chatPriv(pSocket);
}

bool existeUser(char username[256], char user[256]){
	for (int i = 0; i < 30; i++){
		if (strcmp(acceptedSockets[i].username, username) != 0 && strcmp(acceptedSockets[i].username, user) == 0){
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


void chatPriv(struct AcceptedSocket *pSocket){
	while(true){
		recv(pSocket->acceptedSocketFD, buffer, 1024, 0);
		
		if (strcmp(buffer, "_EXIT_") == 0){
			pSocket->convPriv = 0;
			send(pSocket->convPriv, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
			break;
		}
		
		serverLog = fopen("serverLog.txt", "a");
		fprintf(serverLog, "%s %s %s", "Se envio el siguiente mensaje privado -> ", buffer, "\n");
		fclose(serverLog);
		
		send(pSocket->convPriv, buffer, strlen(buffer), 0);
		bzero(buffer, sizeof(buffer));
	}
}

// ===============================================

void nuevoGrupo(struct AcceptedSocket *pSocket){
	char user[256];
	int i = 0;
	
	char users[10][256];
	bool repUser = false;
	int k = 0;
	
	for (int i = 0; i < 10; i++){
		strcpy(users[i], "");
		pSocket->convGrup[i] = 0;
	}

	while (true){
		recv(pSocket->acceptedSocketFD, buffer, 1024, 0);

		strcpy (user, buffer);
		bzero(buffer, sizeof(buffer));
		
		if (strcmp(user, "DONE") == 0){
			break;
		}
		
		for (int j = 0; j < 10; j++){
			if (strcmp(users[j], user) == 0){
				strcpy (buffer, "Usuario NO Encontrado");
				send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
				bzero(buffer, sizeof(buffer));
				repUser = true;
				break;
			}
		}
		
		if (repUser){
			repUser = false;
			continue;
		}
		
		if (existeUser(pSocket->username, user)){
			strcpy (buffer, "Usuario Encontrado");
			send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
			
			pSocket->convGrup[i] = getSocket(pSocket->username, user);
			i++;
			
			strcpy (users[k], user);
			k++;
		} else {
			strcpy (buffer, "Usuario NO Encontrado");
			send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
		}
	}
	
	chatGrup(pSocket);
}

void chatGrup(struct AcceptedSocket *pSocket){
	while(true){
		recv(pSocket->acceptedSocketFD, buffer, 1024, 0);
		
		if (strcmp(buffer, "_EXIT_") == 0){
			send(pSocket->convPriv, buffer, strlen(buffer), 0);
			bzero(buffer, sizeof(buffer));
			break;
		}
		
		for (int i = 0; i < 10; i++){
			if (pSocket->convGrup[i] == 0)
				continue;
			send(pSocket->convGrup[i], buffer, strlen(buffer), 0);
		}
		
		serverLog = fopen("serverLog.txt", "a");
		fprintf(serverLog, "%s %s %s", "Se envio el siguiente mensaje grupal -> ", buffer, "\n");
		fclose(serverLog);
		
		bzero(buffer, sizeof(buffer));
	}
}

// ===============================================

void cantUsuarios(struct AcceptedSocket *pSocket){
	sprintf(buffer, "%d", acceptedSocketsCount);
	
	printf("Se encontraron %s usuarios conectados.\n\n", buffer);
	
	serverLog = fopen("serverLog.txt", "a");
	fprintf(serverLog, "%s %s %s", "El usuario ", pSocket->username, " ha consultado la cantidad de usuarios conectados\n");
	fclose(serverLog);
	
	send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
}

void listaUsuarios(struct AcceptedSocket *pSocket){
	serverLog = fopen("serverLog.txt", "a");
	fprintf(serverLog, "%s %s %s", "El usuario ", pSocket->username, " ha consultado por la lista de usuarios conectados\n");
	fclose(serverLog);

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
	send(pSocket->acceptedSocketFD, buffer, strlen(buffer), 0);
	bzero(buffer, sizeof(buffer));
}



