/*
 * syscall.h
 *
 *	Create
 *		Create a new task with priority and code
 *	MyTid
 *		Get my task id
 *	MyParentTid
 *		Get parent task's id
 *	Pass
 *		Yield to other tasks
 *	Exit
 *		Finish task
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <request.h>

#define PRIORITY_LOW 3
#define PRIORITY_MED 7
#define PRIORITY_HIGH 11

#define NAMESERVER_TID 1

// Task Creation
int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

// Inter-task Communication
int Send(int Tid, void *msg, int msglen, void *reply, int replylen);
int Receive(int *tid, void *msg, int msglen);
int Reply(int tid, void *reply, int replylen);

// Name Server
int RegisterAs(char *name);
int WhoIs(char *name);

// Helper function
int sendRequest(Request* request);

#endif
