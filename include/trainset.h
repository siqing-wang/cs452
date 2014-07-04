// trainset.h
// Author: Yu Meng Zhang
// All commands related with trainset

#ifndef __TRAINSET_H__
#define __TRAINSET_H__

#include <ts7200.h>
#include <io.h>
#include <ui.h>

/* for switches */
#define SWITCH_TOTAL 22

/* for sensors */
#define SENTABLE_SIZE 6
#define SENSOR_RESET_MODE_ON 192
#define SENSOR_SUBSCRIBE_ALL 133

#define TRAIN_NUM 1

struct track_node;

typedef struct TrainSpeedData {
    int trainNum;
    int lastSpeed;
    int targetSpeed;
    int timetick;
    int timeRequiredToAchieveSpeed;
    int timetickWhenHittingSensor;
    int lastSpeedDuration;
} TrainSpeedData;

typedef struct TrainSetData {
    /* Train Speed Table. */
    TrainSpeedData *tstable[TRAIN_NUM];

    /* Switch Table. */
    int swtable[SWITCH_TOTAL];

    /* Sensor List */
    struct track_node* sentable[SENTABLE_SIZE];
    int numSensorPast;
    int lastByte[10];
    int expectTimetick;

    /* Track graph. */
    struct track_node *track;
} TrainSetData;

/* Trainset Control Functons. */
void trainset_init(TrainSetData *data);
void trainset_go();
void trainset_stop();
void trainset_setSpeed(int train_number, int train_speed);
void trainset_reverse(int train_number);
void trainset_turnSwitch(int switch_number, int switch_direction);
void updateSwitchTable(int switch_number, int switch_direction);
void trainset_subscribeSensorFeeds();
int trainset_pullSensorFeeds(TrainSetData *data);

/* Helper Functions. */
int getSwitchIndex(int switch_number);
int getSwitchNumber(int index);
void printSwitchTable(TrainSetData *data);

#endif
