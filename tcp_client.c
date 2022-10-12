#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Global Variables
char username [256];
void solUsername();
bool menu();
void nuevaConv();
void nuevoGrupo();
void cantUsuarios();
void listaUsuarios();

int main(int argc, char const *argv[])
{
	// create a socket
	int network_socket;
	network_socket = socket(AF_INET, SOCK_STREAM, 0);

	// specify an address for the socket
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9001);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));

	// check for error in the connection
	if (connection_status == -1){
		printf("Error connecting to socket \n\n");
		close(network_socket);
		//return 0;
	}

	/*
	// recieve data from the server
	char server_response[256];
	recv(network_socket, &server_response, sizeof(server_response), 0);

	// print out server response
	printf("The server sent: %s\n", server_response);

	// close socket
	close(network_socket);
	*/

	solUsername();
	
	bool salir = false;
	while(salir != true){
		salir = menu();
	}

	printf("Nos vemos!\n");
	close(network_socket);
	return 0;
}

void solUsername(){
	printf("Inserte su Username: ");
	scanf("%s", username);
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
	printf("Nueva conversacion\n");
}

void nuevoGrupo(){
	printf("Nuevo grupo\n");
}

void cantUsuarios(){
	printf("Cantidad de usuarios\n");
}

void listaUsuarios(){
	printf("Lista de usuarios\n");
}