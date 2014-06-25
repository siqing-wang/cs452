// trainset.h
// Author: Yu Meng Zhang
// All commands related with trainset

#ifndef __TRAINSET_H__
#define __TRAINSET_H__

#include <ts7200.h>
#include <io.h>
#include <ui.h>

/* For train control. */
#define STOP 0
#define REVERSE 15

/* for switches */
#define SWITCH_TOTAL 22
#define SWITCH_TURN_OUT 32
#define SWITCH_STRAIGHT 33
#define SWITCH_CURVE 34

/* for sensors */
#define SENTABLE_SIZE 6
#define SENSOR_RESET_MODE_ON 192
#define SENSOR_SUBSCRIBE_ALL 133



typedef struct TrainSetData {
    /* Train Speed Table. */
    int tstable[80];

    /* Switch Table. */
    int swtable[SWITCH_TOTAL];
} TrainSetData;


typedef struct TrainSetSensorData {
    /* Sensor List */
    int sentable[SENTABLE_SIZE];
    int numSensorPast;
    int lastByte[10];
} TrainSetSensorData;

void trainset_init(TrainSetData *data);
void trainset_go();
void trainset_stop();
void trainset_setSpeed(TrainSetData *data, int train_number, int train_speed);
void trainset_reverse(TrainSetData *data, int train_number);
void trainset_turnSwitch(TrainSetData *data, int switch_number, int switch_direction);
void updateSwitchTable(TrainSetData *data, int switch_number);
void trainset_subscribeSensorFeeds();
int trainset_pullSensorFeeds(TrainSetSensorData *data);

void printSwitchTable(TrainSetData *data);

#endif
