/*
 * message.h - define a data structure for inter-task communication
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MSG_MSGONLY 0
#define MSG_MSGANDSTAT 1
#define MSG_REGAS 2
#define MSG_WHOIS 3
#define MSG_TID 4
#define MSG_ERRNO 5

typedef struct Message
{
	int destTid;
	int srcTid;
	int type;
	int msglen;
	void *msg;
	int stat;

} Message;

#endif