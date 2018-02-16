/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "pilot.h"
#include "prose.h"
#include "robot.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum{
  S_FORGET = 0,
	S_IDLE,
	S_RUNNING,
  S_STOP,
  NB_STATE
}State;

typedef enum{
  A_NOP = 0,
  A_SEND_MVT,
  A_SEND_MVT_NULL,
  A_STOP
}TransitionAction;

typedef enum{
  E_SET_VELOCITY,
  E_CHECK_BUMPED,
  E_STOP,
  NB_EVENT
}Event;

typedef struct {
  State destinationState;
  TransitionAction action;
}Transition;

static Transition mySm[NB_STATE][NB_EVENT]={
  [S_IDLE][E_SET_VELOCITY] ={S_RUNNING, A_SEND_MVT},
  [S_RUNNING][E_SET_VELOCITY] ={S_RUNNING, A_SEND_MVT},
  [S_RUNNING][E_CHECK_BUMPED] = {S_IDLE, A_SEND_MVT},
  [S_RUNNING][E_STOP] = {S_STOP, A_STOP},
  [S_IDLE][E_STOP] = {S_STOP, A_STOP},


};
static void Pilot_run(Event event, VelocityVector vel);
State myState;

void Pilot_start(){
	Robot_start();
  myState = S_IDLE;
}

void Pilot_stop(){
	Robot_setVelocity(0, 0);
}

void Pilot_new(){
	Robot_new();
}

void Pilot_free(){
	Robot_free();
}

void Pilot_setVelocity(VelocityVector vel){
  Event event;
  event = E_SET_VELOCITY;
  Pilot_run(event, vel);
}

static void Pilot_performAction(TransitionAction anAction, VelocityVector vel){
	switch (anAction) {
		case  A_NOP:
			break;
		case A_SEND_MVT:
      switch (vel.dir){
      case LEFT:
        Robot_setVelocity(-vel.power, vel.power);
        break;
      case RIGHT:
        Robot_setVelocity(vel.power, -vel.power);
        break;
      case FORWARD:
        Robot_setVelocity(vel.power, vel.power);
        break;
      case BACKWARD:
        Robot_setVelocity(-vel.power, -vel.power);
        break;
      }
			break;

    case A_SEND_MVT_NULL:
      myState = S_IDLE;
      break;
		case A_STOP:
      Pilot_stop();
      myState = S_IDLE;
      break;
	}
}

static void Pilot_run(Event event, VelocityVector vel){
	State aState;
	TransitionAction anAction;
	anAction = mySm[myState][event].action;
	aState = mySm[myState][event].destinationState;

	if(aState != S_FORGET){
		Pilot_performAction(anAction, vel);
		myState = aState;
	}
}

PilotState Pilot_getState(){
	PilotState pilotState;
	pilotState.speed = Robot_getRobotSpeed();
	pilotState.collision = Robot_getSensorState().collision;
	pilotState.luminosity = Robot_getSensorState().luminosity;
	return pilotState;
}

void Pilot_check(){
	SensorState sensorState = Robot_getSensorState();
  if(sensorState.collision == 1){

  }else{

  }
}
