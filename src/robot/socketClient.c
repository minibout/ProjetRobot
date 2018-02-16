#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "socketClient.h"

int mySocket;

int SocketClient_connect(void)
{
	struct sockaddr_in adresse_du_serveur;

	/*On choisit un socket TCP (SOCK_STREAM) sir IP (AP_INET)*/
	mySocket = socket(AF_INET, SOCK_STREAM, 0);
	/*AF_INET = famille TCP/IP*/
	adresse_du_serveur.sin_family = AF_INET;
	/*port du serveur auquel se connecter*/
	adresse_du_serveur.sin_port = htons(PORT_DU_SERVEUR);
	/*adresse IP (ou nom de domaine) du serveur auquel se connecter */
	adresse_du_serveur.sin_addr = *((struct in_addr *)gethostbyname("127.0.0.1")->h_addr_list[0]);
	/*On demande la connexion aupres du serveur*/
	connect(mySocket, (struct sockaddr *)&adresse_du_serveur, sizeof(adresse_du_serveur));
	printf("connected\n");
	return mySocket;

}

void SocketClient_write(char command){
	write(mySocket, &command, sizeof(command));
	printf("wrote %c in %d\n", command, mySocket);

}

void SocketClient_close(){
	close(mySocket);
}
