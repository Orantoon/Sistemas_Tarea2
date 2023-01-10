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
pthread_mutex_t mutex;		// Semaforo

char username [256];
bool grupal = false;

// Solicitar username al conectarse al server
void solUsername();

// Thread separada que esta esperando a recibir mensajes de otros clientes
void threadResp();
void leerResp();

// Menu principal
bool menu();

// Opcion de conversacion privada entre clientes
void nuevaConv();
void menuConv();
void enviarMensaje();
void enviarArchivo();

// Opcion de conversacion grupal entre clientes
void nuevoGrupo();

// Otras consultas de los clientes
void cantUsuarios();
void listaUsuarios();

// recv(client_socket, buffer, 1024, 0);		RECEIVE
// send(client_socket, buffer, sizeof(buffer), 0);	SEND


int main(int argc, char const *argv[])
{
	// ---------- Configuracion de Sockets -----------
	
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
	
	// -------------------------------------------

	solUsername(client_socket);
	threadResp();
	
	bool salir = false;
	while(salir != true){
		salir = menu();
	}


	// close socket
	printf("Nos vemos!\n");
	close(client_socket);
	
	pthread_mutex_destroy(&mutex);
	return 0;
}

// ===============================================

void solUsername(int client_socket){
	printf("Inserte su Username: ");
	scanf("%[^\n]", username);
	send(client_socket, username, sizeof(username), 0);
}

// ===============================================

void threadResp(){
	pthread_t id;
	pthread_create(&id, NULL, leerResp, NULL);
}

void leerResp(){
	char message[1024];
	bzero(message, sizeof(message));
	
	FILE *fp;
	char *filename = "recv.txt";
	
	while (true){
		recv(client_socket, message, 1024, 0);
		
		if (message[0] != '['){	// Se esta enviando un archivo
			//printf(">> %s <<\n\n", message);
			printf("Usted ha recibido un archivo, revise el archivo llamado 'recv.txt' que se ha creado en la carpeta.\n\n");
			
			fp = fopen(filename, "w");
			fprintf(fp, "%s", message);
			bzero(message, sizeof(message));
			fclose(fp);
			continue;
		}
		
		printf("%s\n\n", message);
		bzero(message, sizeof(message));
	}
}

// ===============================================

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

// ===============================================

void nuevaConv(){
	char user [256];
	while (true){
		printf("Escriba el Username de la persona con la que quisiera enviar mensajes: ");
		
		scanf(" %[^\n]", user);
		printf("\n\n");
		
		send(client_socket, user, sizeof(user), 0);
		
		// Recibe la respuesta del servidor de si el usuario existe
		pthread_mutex_lock(&mutex);
		recv(client_socket, buffer, 1024, 0);
		pthread_mutex_unlock(&mutex);
		if (strcmp(buffer, "Usuario Encontrado") == 0){
			bzero(buffer, sizeof(buffer));
			break;
		}
		printf("El usuario que desea contactar no se ha encontrado. Por favor, vuelva a intentarlo.\n");
		bzero(buffer, sizeof(buffer));
	}
	
	printf("\n--- CHAT CON %s ---\n\n", user);

	grupal = false;
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
	
	if (grupal){
		strcpy(buffer, "[Mensaje de Grupo | ");
	} else {
		strcpy(buffer, "[Mensaje Privado | ");
	}
	strcat(buffer, username);
	strcat(buffer, "]: ");
	strcat(buffer, mensaje);
	
	send(client_socket, buffer, sizeof(buffer), 0);
	bzero(buffer, sizeof(buffer));
}

void enviarArchivo(){
	FILE *fp;
	char filename[100];
	
	printf("Escriba el nombre y extension del archivo que desea enviar: ");
	scanf("%s", filename);
	printf("\n\n");
	
	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Error al leer el archivo, revise que escribio el nombre y extension correctos.");
		return;
	}
	
	fgets(buffer, sizeof(buffer), fp);
	if (send(client_socket, buffer, sizeof(buffer), 0) == -1){
		perror("Error al enviar el archivo.");
		return;
	}
	
	printf("El archivo %s se ha enviado correctamente.\n\n", filename);
	bzero(buffer, sizeof(buffer));
}

// ===============================================

void nuevoGrupo(){
	bool hayUsers = false;
	char user [256];
	
	while (true){
		printf("Escriba el Username de una persona que desea agregar al grupo o escriba 'DONE' para finalizar: ");
		
		scanf(" %[^\n]", user);
		printf("\n\n");
		
		send(client_socket, user, sizeof(user), 0);
		
		if (strcmp(user, "DONE") == 0){
			break;
		}

		// Recibe la respuesta del servidor de si el usuario existe
		pthread_mutex_lock(&mutex);
		recv(client_socket, buffer, 1024, 0);
		pthread_mutex_unlock(&mutex);
		if (strcmp(buffer, "Usuario Encontrado") == 0){
			hayUsers = true;
			bzero(buffer, sizeof(buffer));
			bzero(user, sizeof(user));
			continue;
		}
		printf("El usuario que desea contactar no se ha encontrado. Por favor, vuelva a intentarlo.\n");
		bzero(buffer, sizeof(buffer));
		bzero(user, sizeof(user));
	}

	if (!hayUsers){
		printf("No ha ingresado ningun usuario, se devolvera al menu a continuacion.\n\n");
		return;
	}	
	
	grupal = true;
	menuConv();
}

// ===============================================

void cantUsuarios(){
	printf("La cantidad de usuarios conectados es de: ");
	
	pthread_mutex_lock(&mutex);
	recv(client_socket, buffer, 1024, 0);
	pthread_mutex_unlock(&mutex);
	printf("%s\n\n", buffer);
	
	bzero(buffer, sizeof(buffer));
}

void listaUsuarios(){
	printf("La lista de usuarios conectados es:\n\n");
	
	pthread_mutex_lock(&mutex);
	recv(client_socket, buffer, 1024, 0);
	pthread_mutex_unlock(&mutex);
	printf("%s\n\n", buffer);
	
	bzero(buffer, sizeof(buffer));
}




