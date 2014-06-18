/*
 * ioserver.h
 */

#ifndef __IOSERVER_H__
#define __IOSERVER_H__

/* IOserverMessage.type */
#define IOServerMSG_SEND_NOTIFIER   1
#define IOServerMSG_RECV_NOTIFIER   2
#define IOServerMSG_CTRL_NOTIFIER   3
#define IOServerMSG_INCP_NOTIFIER   4
#define IOServerMSG_CLIENT          5

/* IOserverMessage.syscall */
#define IOServerMSG_PUTC            1
#define IOServerMSG_GETC            2

/* Message sent/received by IO server. */
typedef struct IOserverMessage
{
    int type;           // Notifier/Client
    int syscall;        // Delay, Time, Until
    int data;

} IOserverMessage;

/* IOserver task code. */
void terminalIOServer();
void trainIOServer();

#endif
