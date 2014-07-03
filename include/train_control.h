/*
 * train_control.h
 */

#ifndef __TRAIN_CONTROL_H__
#define __TRAIN_CONTROL_H__

#define COURIER_NUM_MAX     6

typedef enum {
    TRAINCTRL_TR_SETSPEED,
    TRAINCTRL_TR_REVERSE,
    TRAINCTRL_TR_STOPAT,
    TRAINCTRL_SW_CHANGE,
    TRAINCTRL_SEN_TRIGGERED,
    TRAINCTRL_UNDRAW,
    TRAINCTRL_UNDRAW_COMPLETE,
    TRAINCTRL_DRAW,
    TRAINCTRL_UPDATE_TSTABLE,
    TRAINCTRL_HALT,
    TRAINCTRL_COURIER_FREE,
} trainControl_type;

/* Message sent/received by IO server. */
typedef struct TrainControlMessage
{
    int destTid;
    trainControl_type type;
    int num;
    int data;
    int data2;
    char *location;

} TrainControlMessage;

typedef struct CouriersStatus
{
    int courierTable[COURIER_NUM_MAX];
    int courierStartIndex;
    int courierAvailable;
} CouriersStatus;

void trainControlServer();

#endif
