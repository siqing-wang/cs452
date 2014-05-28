/*
 * message.h - define a data structure for inter-task communication
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MSG_STRING 0
#define MSG_REGAS 1
#define MSG_WHOIS 2
#define MSG_TID 3

typedef struct Message
{
	int destTid;
	int srcTid;
	int type;
	int msglen;
	void *msg;
	int replylen;

} Message;

#endif
