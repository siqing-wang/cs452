// trainset.h
// Author: Yu Meng Zhang
// All commands related with trainset

#ifndef __TRAINSET_H__
#define __TRAINSET_H__

#include <ts7200.h>
#include <io.h>
#include <ui.h>


#define STOP 0
#define REVERSE 15

// for switches
#define SWITCH_TOTAL 22
#define SWITCH_TURN_OUT 32
#define SWITCH_STRAIGHT 33
#define SWITCH_CURVE 34

// for sensors
#define SENSOR_RESET_MODE_ON 192
#define SENSOR_SUBSCRIBE_ALL 133

void trainset_init(int *trainSpeedTable, int *switchTable, int *sensorTable);
void trainset_go();
void trainset_stop();
void trainset_setSpeed(int train_number, int train_speed){}
void trainset_reverse(int train_number){}
void trainset_turnSwitch(int switch_number, int switch_direction){}
void updateSwitchTable(int switch_number){}
void trainset_subscribeSensorFeeds(){}
int trainset_pullSensorFeeds(){return 0;}

void printSwitchTable();

#endif
