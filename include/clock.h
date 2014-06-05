/*
 * clock.h
 */

#ifndef __CLOCK_H__
#define __CLOCK_H__

#define CServerMSG_NOTIFIER 1
#define CServerMSG_CLIENT   2

/* Used by name server. */
typedef struct ClockserverMessage
{
    int type;
    int data;

} ClockserverMessage;

void clockServer();

#endif
