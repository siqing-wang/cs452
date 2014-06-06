/*
 * syscall.h
 * ---------------------------------------------------------------------------
 * Task Creation
 * ---------------------------------------------------------------------------
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
 *
 * ---------------------------------------------------------------------------
 * Inter-task Communication
 * ---------------------------------------------------------------------------
 *  Send
 *      Send a message to another task, task will be blocked until reply is
 *      received.
 *  Receive
 *      Receive msg from send queue. Will be blocked if no msg available.
 *  Reply
 *      Reply to a task waiting for reply.
 *
 * ---------------------------------------------------------------------------
 * Interrupt Processing
 * ---------------------------------------------------------------------------
 *  AwaitEvent
 *      Block a task on an event.
 *
 * ---------------------------------------------------------------------------
 * Name Server
 * ---------------------------------------------------------------------------
 *  RegisterAs
 *      Register task as a service.
 *  WhoIs
 *      Get tid of the task registered to provide given service.
 *
 * ---------------------------------------------------------------------------
 * Clock Server
 * ---------------------------------------------------------------------------
 *  Delay
 *      Block task for given number of clock ticks.
 *  Time
 *      Get current timer ticks from clock server.
 *  DelayUntil
 *      Block task until timer has ticked given times.
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <request.h>

#define PRIORITY_LOW 3
#define PRIORITY_MED 7
#define PRIORITY_HIGH 11

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

// Interrupt Processing
int AwaitEvent(int eventid);

// Name Server
int RegisterAs(char *name);
int WhoIs(char *name);

// Clock Server
int Delay(int ticks);
int Time();
int DelayUntil(int ticks);

#endif
