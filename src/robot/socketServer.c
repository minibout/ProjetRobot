#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "socketServer.h"
#include "pilot.h"

#define MAX_PENDING_CONNECTIONS (5)

int connexion = 5;
int run = 1;

void AdminUI_useCommande(char commande, int socket){
  VelocityVector vel;
	PilotState pilotState;

  switch(commande){
  case 'z':
  	vel.dir = FORWARD;
  	vel.power = 100;
  	Pilot_setVelocity(vel);
  	break;

  case 's':
  	vel.dir = BACKWARD;
  	vel.power = 100;
  	Pilot_setVelocity(vel);
  	break;

  case 'q':
  	vel.dir = LEFT;
  	vel.power = 100;
  	Pilot_setVelocity(vel);
  	break;

  case 'd':
  	vel.dir = RIGHT;
  	vel.power = 100;
  	Pilot_setVelocity(vel);
  	break;

  case ' ':
  	vel.dir = FORWARD;
  	vel.power = 0;
  	Pilot_setVelocity(vel);
  	break;

  case 'a':
    Pilot_stop();
    run = 0;
  	break;

  case 'r':
  	pilotState = Pilot_getState();
    write(socket, &pilotState.speed, sizeof(pilotState.speed));
    write(socket, &pilotState.collision, sizeof(pilotState.collision));
    write(socket, &pilotState.luminosity, sizeof(pilotState.luminosity));
  	break;

  case 'e':
  	break;

  default:
    break;
	}
}

void communication_avec_client(int socket)
{
  char commande;
  printf("Before read\n");

  while(run){
	  	read(socket, &commande, sizeof(commande));

		//commande = ntohl (commande);
		printf("I have read this : ");

		printf("%c\n",commande);

		AdminUI_useCommande(commande, socket);
  }



}

void SocketServer_start()
{
	int socket_ecoute;
	int socket_donnees;
	struct sockaddr_in mon_adresse;


	/* Creation du socket : AF_INET = IP, SOCK_STREAM = TCP */
	socket_ecoute = socket (AF_INET, SOCK_STREAM, 0);
	mon_adresse.sin_family = AF_INET; /*Type d'adresse = IP*/
	mon_adresse.sin_port = htons(PORT_DU_SERVEUR);/*Port TCP ou le serviece est accessible*/
	mon_adresse.sin_addr.s_addr = htonl(INADDR_ANY);/*On s'attache a toutes les interfaces*/
	/* on attache le socket a l'adresse indiquee */
	bind(socket_ecoute, (struct sockaddr *)&mon_adresse, sizeof (mon_adresse));
	/*Mise en ecoute du socket*/
	listen(socket_ecoute, MAX_PENDING_CONNECTIONS);
	printf("listen to connection\n");
	while(1)
	{
		/*acceptation de la connexion*/
	socket_donnees = accept(socket_ecoute, NULL, 0);
    printf("connected\n");

    Pilot_new();
    Pilot_start();

		if(fork() == 0){
			printf("fork passed\n");
			communication_avec_client(socket_donnees);
		}
	}
	/*On ferme le port sur lequel on ecoutait*/
	close(socket_ecoute);
}
