/*
 * train_control.h
 */

#ifndef __TRAIN_CONTROL_H__
#define __TRAIN_CONTROL_H__

#define COURIER_NUM_MAX     30
#define TIME_INTERVAL       5

typedef enum {
    TRAINCTRL_INIT,
    TRAINCTRL_INIT_COMPLETE,
    TRAINCTRL_TR_SETSPEED,
    TRAINCTRL_TR_STOP,
    TRAINCTRL_TR_REVERSE,
    TRAINCTRL_TR_REVERSE_COMPLETE,
    TRAINCTRL_TR_GO,
    TRAINCTRL_TR_STOPAT,
    TRAINCTRL_SW_CHANGE,
    TRAINCTRL_SEN_TRIGGERED,
    TRAINCTRL_UPDATE_TSTABLE,
    TRAINCTRL_HALT,
    TRAINCTRL_HALT_COMPLETE,
    TRAINCTRL_COURIER_FREE,
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
