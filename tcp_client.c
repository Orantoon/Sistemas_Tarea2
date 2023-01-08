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

int client_socket;

char username [256];

void solUsername();
void threadResp();
void leerResp();

bool menu();
void nuevaConv();

void menuConv();
void enviarMensaje();
void enviarArchivo();

void nuevoGrupo();
void cantUsuarios();
void listaUsuarios();

// recv(client_socket, buffer, 1024, 0);		RECEIVE
// send(client_socket, buffer, sizeof(buffer), 0);	SEND

int main(int argc, char const *argv[])
{
	// create a socket
	int ret;
	
	struct sockaddr_in server_address;
	
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0){
		printf("Error connecting to socket.\n\n");
		exit(1);
	}
	
	printf("Client Socket created successfully!\n\n");
	
	// specify an address for the socket
	memset(&server_address, '\0', sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;
	
	int yes = 1;
	if (setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
		perror("setsockopt");
		exit(1);
	}	
	
	// connect to server
	ret = connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address));
	if (ret < 0){
		printf("Error in the connection.\n\n");
		exit(1);
	}
	
	printf("Client connected to server.\n\n");
	
	// ===============================================

	solUsername(client_socket);
	threadResp();
	
	bool salir = false;
	while(salir != true){
		salir = menu();
	}


	// close socket
	printf("Nos vemos!\n");
	close(client_socket);
	return 0;
}

void solUsername(int client_socket){
	printf("Inserte su Username: ");
	scanf("%[^\n]", username);
	send(client_socket, username, sizeof(username), 0);
}

void threadResp(){
	pthread_t id;
	pthread_create(&id, NULL, leerResp, NULL);
}

void leerResp(){
	char message[1024];
	bzero(message, sizeof(message));
	while (true){
		recv(client_socket, message, 1024, 0);
		
		printf("%s\n\n", message);
		bzero(message, sizeof(message));
	}
}

bool menu(){
	int opcion;

	printf("\n=== MENU ===\n");
	printf("Hola %s!\n", username);
	printf("Seleccione una de las siguientes opciones:\n\n");

	printf("1. Iniciar nueva conversacion.\n");
	printf("2. Crear un nuevo grupo.\n");
	printf("3. Cantidad de usuarios conectados.\n");
	printf("4. Lista de usuarios conectados.\n");
	printf("5. Salir.\n\n");

	scanf("%d", &opcion);
	printf("\n");
	
	// The option is sent to the server so it knows what to do too
	sprintf(buffer, "%d", opcion);
	
	send(client_socket, buffer, sizeof(buffer), 0);
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
			break;
		case 5:
			return true;
	}

	return false;
}


void nuevaConv(){
	char user [256];
	while (true){
		printf("Escriba el Username de la persona con la que quisiera iniciar una nueva conversacion: ");
		
		scanf(" %[^\n]", user);
		printf("\n\n");
		
		send(client_socket, user, sizeof(user), 0);
		
		// Recibe la respuesta del servidor de si el usuario existe
		recv(client_socket, buffer, 1024, 0);
		if (strcmp(buffer, "Usuario Encontrado") == 0){
			bzero(buffer, sizeof(buffer));
			break;
		}
		printf("El usuario que desea contactar no se ha encontrado. Por favor, vuelva a intentarlo.\n");
		bzero(buffer, sizeof(buffer));
	}
	
	printf("\n--- CHAT CON %s ---\n\n", user);

	menuConv();
}

void menuConv(){
	bool salir = false;
	int op;
	
	while (!salir){
		
		printf("---------------------------------\n");
		printf("Seleccione una accion a realizar:\n\n");

		printf("1. Mandar un mensaje de texto.\n");
		printf("2. Enviar un archivo.\n");
		printf("3. Abandonar conversacion.\n\n");

		scanf("%d", &op);
		printf("\n\n");
		
		switch(op){
			case 1:
				enviarMensaje();
				break;
			case 2:
				enviarArchivo();
				break;
			case 3:
				strcpy(buffer, "_EXIT_");
				send(client_socket, buffer, sizeof(buffer), 0);
				bzero(buffer, sizeof(buffer));
				salir = true;
		}
	}
}

void enviarMensaje(){
	char mensaje [1024];
	scanf(" %[^\n]", mensaje);
	printf("\n\n");
	
	strcpy(buffer, "[Mensaje Privado | ");
	strcat(buffer, username);
	strcat(buffer, "]: ");
	strcat(buffer, mensaje);
	
	send(client_socket, buffer, sizeof(buffer), 0);
	bzero(buffer, sizeof(buffer));
}

void enviarArchivo(){ // =================================================

}

void nuevoGrupo(){
	printf("Nuevo grupo\n");
}

void cantUsuarios(){
	printf("La cantidad de usuarios conectados es de: ");
	
	recv(client_socket, buffer, 1024, 0);
	printf("%s\n\n", buffer);
	
	bzero(buffer, sizeof(buffer));
}

void listaUsuarios(){
	printf("La lista de usuarios conectados es:\n\n");
	
	recv(client_socket, buffer, 1024, 0);
	printf("%s\n\n", buffer);
	
	bzero(buffer, sizeof(buffer));
}




