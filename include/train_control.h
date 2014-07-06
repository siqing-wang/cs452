/*
 * train_control.h
 */

#ifndef __TRAIN_CONTROL_H__
#define __TRAIN_CONTROL_H__

#define COURIER_NUM_MAX     15

typedef enum {
    TRAINCTRL_INIT,
    TRAINCTRL_INIT_COMPLETE,
    TRAINCTRL_TR_SETSPEED,
    TRAINCTRL_TR_REVERSE,
    TRAINCTRL_TR_REVERSE_COMPLETE,
    TRAINCTRL_TR_STOPAT,
    TRAINCTRL_SW_CHANGE,
    TRAINCTRL_SW_CHANGE_UNDRAW,
    TRAINCTRL_SW_CHANGE_DRAW,
    TRAINCTRL_SEN_TRIGGERED,
    TRAINCTRL_SEN_TRIGGERED_UNDRAW,
    TRAINCTRL_SEN_TRIGGERED_DRAW,
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
    int data2;
    int delay;
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
