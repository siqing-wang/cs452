/*
 * train_control.h
 */

#ifndef __TRAIN_CONTROL_H__
#define __TRAIN_CONTROL_H__

#define COURIER_NUM_MAX     30
#define TIME_INTERVAL       5

#define TR_IDENTITY_POLICE  2
#define TR_IDENTITY_THIEF   1

typedef enum {
    TRAINCTRL_TR_INIT,
    TRAINCTRL_TR_SETSPEED,
    TRAINCTRL_TR_STOP,
    TRAINCTRL_TR_REVERSE,
    TRAINCTRL_TR_GO,
    TRAINCTRL_SW_CHANGE,
    TRAINCTRL_HALT,
    TRAINCTRL_HALT_COMPLETE,
    TRAINCTRL_COURIER_FREE,
    TRAINCTRL_TR_IDENTITY,
} trainControl_type;

/* Message sent/received by IO server. */
typedef struct TrainControlMessage
{
    int destTid;
    trainControl_type type;
    int num;
    int data;
    int delay;
    char location[6];

} TrainControlMessage;

typedef struct CouriersStatus
{
    int courierTable[COURIER_NUM_MAX];
    int courierStartIndex;
    int courierAvailable;
} CouriersStatus;

void trainControlServer();

#endif
