/*
 * message.h - define a data structure for inter-task communication
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

/* Goes in and out userspace and kernel space as part of a request. */
typedef struct Message
{
    int destTid;
    int srcTid;

    int sendMsgLen;
    void *sendMsg;

    int rcvMsgLen;
    int rcvMsgOrigLen;
    void *rcvMsg;

} Message;

#endif
