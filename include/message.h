/*
 * message.h - define a data structure for inter-task communication
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define SERVERNAME_MAX_LENGTH 64

#define NServerMSG_REGAS 0
#define NServerMSG_WHOIS 1

#define RPSMSG_SIGNUP 0
#define RPSMSG_PLAY 1
#define RPSMSG_QUIT 2
#define RPSMSG_ROCK 1
#define RPSMSG_PAPER 2
#define RPSMSG_SCISSORS 3

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

typedef struct RPSMessage
{
    int type;
    int choice;

} RPSMessage;

#endif
