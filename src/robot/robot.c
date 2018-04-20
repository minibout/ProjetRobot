/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */  


#include "robot.h"
#include "prose.h"

#include <stdio.h>
#include <stdlib.h>

#define ROBOT_CMD_STOP (0)

#define LEFT_MOTOR MD
#define RIGHT_MOTOR MA
#define LIGHT_SENSOR S1
#define FRONT_BUMPER S3
#define FLOOR_SENSOR S2

// a completer

Robot *robot;

void Robot_start(){
	if (ProSE_Intox_init("127.0.0.1", 12345) == -1) {
			PProseError("Problème d'initialisation du simulateur Intox");
	}
	
	robot->mL = Motor_open(LEFT_MOTOR);
	if(robot->mL == NULL){
		PProseError("Problème d'ouverture du moteur droit (port MD)");
	}
	
	robot->mR = Motor_open(RIGHT_MOTOR);
	if(robot->mR == NULL){
		PProseError("Problème d'ouverture du moteur droit (port MA)");
	}
	
	robot->contactSensor = ContactSensor_open(FRONT_BUMPER);
	if(robot->contactSensor == NULL){
			PProseError("Problème d'ouverture du moteur droit (port S3)");
	}
	
	robot->lightSensor = LightSensor_open(LIGHT_SENSOR);
	if(robot->lightSensor == NULL){
			PProseError("Problème d'ouverture du moteur droit (port S1)");
	}
	
}

void Robot_stop()
{
	Motor_close(robot->mL);
	Motor_close(robot->mR);
	ContactSensor_close(robot->contactSensor);
	LightSensor_close(robot->lightSensor);
	ProSE_Intox_close();

}

void Robot_new()
{
	robot = (Robot *)malloc(sizeof(Robot));
}

void Robot_free(){
    free(robot);
}

void Robot_setVelocity(int mr,int ml)
{
	
	if(Motor_setCmd(robot->mL, ml) == -1){
		printf("Probleme \n");

		PProseError("Problème de commande du moteur droit");
		
	}
	
	if(Motor_setCmd(robot->mR, mr) == -1){
		PProseError("Problème de commande du moteur gauche");
	}
}

int Robot_getRobotSpeed()
{
	int vR = Motor_getCmd(robot->mL);
	int vL = Motor_getCmd(robot->mR);

	return (vR + vL)/2;
}

SensorState Robot_getSensorState()
{
	SensorState sensorState;
	sensorState.collision = ContactSensor_getStatus(robot->contactSensor);
	sensorState.luminosity = (float)LightSensor_getStatus(robot->lightSensor);
	return sensorState;
}

bool_e Robot_hasBumped(void)
{
  if (Robot_getSensorState().collision == BUMPED)
  {
    return TRUE;
  } else {
    return FALSE;
  }
}
