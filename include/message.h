/*
 * message.h - define a data structure for inter-task communication
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

typedef struct Message
{
    int destTid;
    int srcTid;
    int msglen;
    void *msg;
    int replylen;

} Message;

#endif
