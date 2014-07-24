// trainset.h
// Author: Yu Meng Zhang
// All commands related with trainset

#ifndef __TRAINSET_H__
#define __TRAINSET_H__

#include <ts7200.h>
#include <io.h>
#include <ui.h>
#include <lock.h>

/* for switches */
#define SWITCH_TOTAL 22

/* for sensors */
#define SENTABLE_SIZE 6
#define SENSOR_RESET_MODE_ON 192
#define SENSOR_SUBSCRIBE_ALL 133

#define TRAIN_NUM 3
#define TRAIN_LENGTH 210
#define TIMEOUT 500

#define REVERSE_GAP 300

#define RIGHT_DIR 1
#define WRONG_DIR 2

struct track_node;

typedef struct TrainData {
    /* General*/
    int trainNum;
    int reverse;
    int reverseInProgress;
    int stopInProgress;
    int shortMoveInProgress;
    int init;

    /* Speed */
    int lastSpeed;
    int targetSpeed;
    int timetickSinceSpeedChange;
    int delayRequiredToAchieveSpeed;

    /* Current Location */
    struct track_node *lastSensor;
    struct track_node *lastLandmark;
    double distanceAfterLastLandmark;
    int timetickWhenHittingSensor;

    /* Sensor */
    int numSensorPast;
    struct track_node *lastLastSensor;
    int estimateTimetickHittingLastSensor;
    int actualTimetickHittingLastSensor;
    struct track_node *nextSensor;
    int expectTimetickHittingNextSensor;
    struct track_node *nextNextSensor;
    int expectTimetickHittingNextNextSensor;

    /* Broken switch detection */
    struct track_node *nextWrongSensor;
    struct track_node *nextSwitch;

    /* StopAt related */
    int needToStop;
    int delayToStop;
    int continueToStop;
    unsigned int stopAtSwDirctions;
    unsigned int stopAtSwInvolved;
    struct track_node *nextLocation;
    int nextLocationOffset;
    struct track_node *finalLocation;
    struct track_node *finalLocationAlt;
    int finalLocationOffset;
} TrainData;

typedef struct TrainSetData {
    /* Train Speed Table. */
    TrainData *trtable[TRAIN_NUM];
    Lock *trtableLock[TRAIN_NUM];

    /* Switch Table. */
    int swtable[SWITCH_TOTAL];
    Lock *swtableLock;

    /* Sensor List */
    struct track_node* sentable[SENTABLE_SIZE];
    int totalSensorPast;
    int lastByte[10];

    /* Track graph. */
    struct track_node *track;
    Lock *trackLock;
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
