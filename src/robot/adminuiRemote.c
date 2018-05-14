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

typedef int IdScreen;
typedef int Indice;

volatile int run;

Indice previousEventNumber;
Indice currentEventNumber;
IdScreen myCurrentScreen;


/*
 * paramètres de configuration du pilote
 */
#define NAME_MQ_BOX  "/bal_adminui" //ne pas oublier le / devant le nom !
#define MQ_MSG_COUNT 10 // min et max variable pour Linux (suivant version noyau, cf doc)
#define MQ_MSG_SIZE 256 // 128 est le minimum absolu pour Linux, 1024 une valeur souvent utilisée !

// On commence toujours par l'état S_FORGET, suivi de l'état initial de la MAE
#define STATE_GENERATION S(S_FORGET) S(S_MAIN_SCREEN) S(S_LOG_SCREEN) S(S_DEATH)
#define S(x) x,
typedef enum {STATE_GENERATION STATE_NB} State;
#undef S
#define S(x) #x,
const char * const State_Name[] = { STATE_GENERATION };
#undef STATE_GENERATION
#undef S
static const char * State_getName(int i)
{
    return State_Name[i];
}

//On commence toujours par A_NOP : pas d'action à faire
#define ACTION_GENERATION S(A_NOP) S(A_DISPLAY) S(A_SET_VEL)  S(A_ROBOT) S(A_STOP)
#define S(x) x,
typedef enum {ACTION_GENERATION ACTION_NB}  Action;
#undef S
#define S(x) #x,
const char * const Action_Name[] = { ACTION_GENERATION };
#undef ACTION_GENERATION
#undef S

static const char * Action_getName(int i)
{
    return Action_Name[i];
}


#define EVENT_GENERATION S(E_) S(E_BOUTOFF) S(E_STOP)
#define S(x) x,
typedef enum {EVENT_GENERATION EVENT_NB}  Event;
#undef S
#define S(x) #x,
const char * const EventName[] = { EVENT_GENERATION };
#undef EVENT_GENERATION
#undef S

static const char * Event_getName(int i)
{
    return EventName[i];
}


typedef struct
{
	State destinationState;
	Action action;
} Transition;

typedef struct
{
	Event event;
// définition générique pour permettre l'ajout d'attributs dans les messages si besoin
// Ajout des attributs si nécessaire pour les actions
} MqMsg;

// Attributs de Lampe (singleton)
static State MyState;
static pthread_t MyThread;

/* MAE de Lampe */
/* on ne met que les transitions induisant des changements d'états */

static Transition MySm [STATE_NB][EVENT_NB] =
{
    [S_OFF][E_BOUTON]= {S_ON,A_LIGHT_ON},
    [S_ON][E_BOUTOFF]={S_OFF,A_LIGHT_OFF},
    [S_OFF][E_STOP]= {S_DEATH,A_STOP},
    [S_ON][E_STOP]= {S_DEATH,A_STOP},
};

//Methode permettant de vérifier la MAE en générant le code plantUML de MySm
static void Lampe_checkMAE();


typedef union
{
	char buffer[MQ_MSG_SIZE];
	MqMsg data;
} MqMsgAdapter;

MqMsg Lampe_mqReceive ()
{
	int check;
	mqd_t mq;
	MqMsgAdapter msg;
	mq = mq_open (NAME_MQ_BOX, O_RDONLY);
	STOP_ON_ERROR(mq == -1);
	check = mq_receive (mq, msg.buffer, MQ_MSG_SIZE, 0);
	STOP_ON_ERROR(check != sizeof (msg.buffer));
	check = mq_close (mq);
	STOP_ON_ERROR(check != 0);
	return msg.data;
}

static void Lampe_mqSend (MqMsg aMsg)
{
	int check;
	MqMsgAdapter msg;
	mqd_t mq;
	msg.data = aMsg;
	/* envoi de l'ordre à travers la BAL */
	mq = mq_open (NAME_MQ_BOX, O_WRONLY);
	STOP_ON_ERROR(mq == -1);
	check = mq_send (mq, msg.buffer, sizeof (msg.buffer), 0);
	STOP_ON_ERROR(check != 0);
	check = mq_close (mq);
	STOP_ON_ERROR(check != 0);
}

extern void Lampe_boutON ()
{
	MqMsg msg =  {.event = E_BOUTON};
	TRACE("[Lampe] demande d'allumage de la lampe  \n");
	Lampe_mqSend (msg);
}

extern void Lampe_boutOFF ()
{
	MqMsg msg = {.event = E_BOUTOFF};
	TRACE("[Lampe] demande d'extinction de la lampe  \n");
	Lampe_mqSend (msg);
}

extern void Lampe_stop ()
{
	MqMsg msg = {.event = E_STOP};
	TRACE("[Lampe] demande d'arret de la lampe  \n");
	Lampe_mqSend (msg);
}

/* version temporaire en attendant l'utilisation de pointeurs de fonctions */
static void Lampe_performAction (Action anAction)
{
	switch (anAction)
	{
    case A_LIGHT_ON:
        Ampoule_allumer ();
        break;
    case A_LIGHT_OFF:
        Ampoule_eteindre ();
        break;
    case A_STOP:
        break;
    default:
        TRACE("[LAMPE] Action inconnue, pb ds la MAE \n ");
        break;
	}
}

static void* Lampe_run ()
{
	MqMsg msg;
	Action act;

	while (MyState != S_DEATH)
	{
		msg = Lampe_mqReceive ();
		if (MySm[MyState][msg.event].destinationState == S_FORGET) // aucun état de destination, on ne fait rien
		{
            // on trace juste l'évènement détruit
			TRACE("[Lampe] MAE, perte evenement %s  \n", Event_getName(msg.event));
		}
		else /* on tire la transition */
		{
			TRACE("[Lampe] MAE, traitement evenement %s \n",  Event_getName(msg.event));
			act = MySm[MyState][msg.event].action;
			TRACE("[Lampe] MAE, traitement action %s \n", Action_getName(act));
			Lampe_performAction (act);
			MyState = MySm[MyState][msg.event].destinationState;
            TRACE("[Lampe] MAE, va dans etat %s \n", State_getName(MyState));
		}
	}
	Ampoule_free();
	Lampe_free ();
	return (0);
}

extern void Lampe_new ()
{
	struct mq_attr mqa;
	int check;
	mqd_t mq;
    TRACE("[Lampe] Construction \n");

	/* creation de la BAL */
	mqa.mq_maxmsg = MQ_MSG_COUNT;
	mqa.mq_msgsize = MQ_MSG_SIZE;
	mqa.mq_flags = 0;
// Destruction de la BAL au cas ou (si BAL mal détruite lors d'une précédente utilisation)
    check = mq_unlink (NAME_MQ_BOX);
//	STOP_ON_ERROR(check != 0);
	mq = mq_open (NAME_MQ_BOX, O_CREAT, 0777, &mqa);
	STOP_ON_ERROR(mq == -1);

	check = mq_close (mq);
	STOP_ON_ERROR(check != 0);
 	MyState = S_OFF;
    TRACE("[Lampe] Demande construction ampoule\n");
    Ampoule_new();
	check = pthread_create (&MyThread, NULL, Lampe_run, NULL);
	STOP_ON_ERROR(check != 0);
    //Lampe_checkMAE();
}

extern void Lampe_free ()
{
	int check;
	/* destruction de la BAL */
	check = mq_unlink (NAME_MQ_BOX);
	STOP_ON_ERROR(check != 0);
	TRACE("[Lampe] Destruction \n");
}



static void Lampe_checkMAE()
{
    int istate,ievent;
    FILE *stdPlant;
    stdPlant=fopen("MAE_Lampe.puml","w+");
    STOP_ON_ERROR (stdPlant==NULL);
    fprintf(stdPlant, "%s\n", "@startuml");
    //on suppose que l'état initial est toujours le premier donné dans le tableau (indice 1)
    fprintf(stdPlant,"[*] -->%s\n", State_getName(1));
/// on parcoure ensuite toutes les transitions effectives de la MAE et on les traduits en plantUML
    for(istate=0;istate<STATE_NB;istate++)
    {
        for(ievent=0;ievent<EVENT_NB;ievent++)
        {
            // On ne prend que les transitions ayant un effet (chgt d'état et/ou action à faire)
            if(MySm[istate][ievent].destinationState!=S_FORGET)
            {
                fprintf(stdPlant,"%s --> %s: %s / %s \n",
                        State_getName(istate),
                        State_getName(MySm[istate][ievent].destinationState),
                        Event_getName(ievent),
                        Action_getName(MySm[istate][ievent].action ) ) ;
            }
        }
    }
    fprintf(stdPlant, "%s\n", "@enduml");
    fclose(stdPlant);
}


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
