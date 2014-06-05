/*
 * clock.h
 */

#ifndef __CLOCK_H__
#define __CLOCK_H__

#define CLOCKSERVER_TID 	2

#define CServerMSG_NOTIFIER 1
#define CServerMSG_CLIENT   2

#define CServerMSG_DELAY   	1
#define CServerMSG_TIME   	2
#define CServerMSG_UNTIL   	3

/* Used by name server. */
typedef struct ClockserverMessage
{
    int type;
    int syscall;
    int data;

} ClockserverMessage;

void clockServer();

#endif
