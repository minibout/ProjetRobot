/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// a completer


#include "adminui.h"
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
	SocketClient_connect();

	while(run){
		display();
		printf("test");

		char input;
		input = captureChoice();
		SocketClient_write(input);
	}
}

void AdminUI_stop(){
	SocketClient_close();

}

void AdminUI_new(){
}

void AdminUI_free(){
}
