/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// a completer


#include "adminuiRemote.h"

#include "prose.h"
#include "socketClient.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

volatile int run;


void display(){
	printf("Vous pouvez faire les actions suivantes : \n \
			q:aller à gauche \n \
			d:aller à droite \n \
			z:avancer \n \
			s:reculer \n \
			space :stopper \n \
			e:effacer les log \n \
			r:afficher l'état du robot \n \
			a:quitter \n\n\n");
}

int captureChoice(void) {
      int c=0;

      struct termios org_opts, new_opts;
      int res=0;

      res=tcgetattr(STDIN_FILENO, &org_opts);
      assert(res==0);

      memcpy(&new_opts, &org_opts, sizeof(new_opts));
      new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
      tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
      c=getchar();


      res=tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
      assert(res==0);
      return(c);
}

void AdminUI_start(){
	run = 1;
	int mySocket = SocketClient_connect();

	while(run){
		display();

		char input;
		input = captureChoice();

		printf("you wrote %c \n", input);
		SocketClient_write(input);

		if(input == 'a'){
			run = 0;
		}else if(input == 'r'){
			int speed;
			int collision;
			float luminosity;
		  	read(mySocket, &speed, sizeof(speed));
		  	read(mySocket, &collision, sizeof(collision));
		  	read(mySocket, &luminosity, sizeof(luminosity));

		  	printf("speed : %d\ncollision : %d\nluminosity : %f\n", speed, collision, luminosity);

		}else if (input == 'e'){
			printf("\e[1;1H\e[2J");
		}

	}
}

void AdminUI_stop(){
	SocketClient_close();

}

void AdminUI_new(){
}

void AdminUI_free(){
}
