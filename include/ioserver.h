/*
 * ioserver.h
 */

#ifndef __IOSERVER_H__
#define __IOSERVER_H__

#define TRAINIOSERVER_TID           4
#define TERMINALIOSERVER_TID        7

/* IOserverMessage.type */
#define IOServerMSG_SEND_NOTIFIER   1
#define IOServerMSG_RECV_NOTIFIER   2
#define IOServerMSG_CLIENT          3

/* IOserverMessage.syscall */
#define IOServerMSG_PUTC            1
#define IOServerMSG_GETC            2
#define IOServerMSG_PUTSTR          3
#define IOServerMSG_IOIDLE          4

/* Message sent/received by IO server. */
typedef struct IOserverMessage
{
    int type;           // Notifier/Client
    int syscall;        // PutC, GetC
    char data;
    char *str;
    int strSize;

} IOserverMessage;

/* IOserver task code. */
void terminalIOServer();
void trainIOServer();

#endif
