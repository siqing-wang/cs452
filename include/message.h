/*
 * message.h - define a data structure for inter-task communication
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define SERVERNAME_MAX_LENGTH 64

#define NServerMSG_REGAS 0
#define NServerMSG_WHOIS 1


typedef struct Message
{
    int destTid;
    int srcTid;
    int msglen;
    void *msg;
    int replylen;

} Message;

typedef struct NameserverMessage
{
    int type;
    char serverName[SERVERNAME_MAX_LENGTH];

} NameserverMessage;

#endif
