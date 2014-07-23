/*
 * clockserver.h
 */

#ifndef __CLOCKSERVER_H__
#define __CLOCKSERVER_H__

#define CLOCKSERVER_TID     2

/* ClockserverMessage.type */
#define CServerMSG_NOTIFIER 1
#define CServerMSG_CLIENT   2

/* ClockserverMessage.syscall */
#define CServerMSG_DELAY    1
#define CServerMSG_TIME     2
#define CServerMSG_UNTIL    3

/* Message sent/received by clock server. */
typedef struct ClockserverMessage
{
    int type;           // Notifier/Client
    int syscall;        // Delay, Time, Until
    int data;

} ClockserverMessage;

/* Clock server task code. */
void clockServer();

#endif
