/*
 * train_control.h
 */

#ifndef __TRAIN_CONTROL_H__
#define __TRAIN_CONTROL_H__

typedef enum {
    TRAINCTRL_TR_SETSPEED,
    TRAINCTRL_TR_REVERSE,
    TRAINCTRL_TR_STOPAT,
    TRAINCTRL_SW_CHANGE,
    TRAINCTRL_SEN_TRIGGERED,
    TRAINCTRL_UPDATE_TSTABLE,
    TRAINCTRL_HALT,
    TRAINCTRL_HALT_COMPLETE,
} trainControl_type;

/* Message sent/received by IO server. */
typedef struct TrainControlMessage
{
    trainControl_type type;
    int num;
    int data;
    int data2;
    char *location;

} TrainControlMessage;

void trainControlServer();

#endif
