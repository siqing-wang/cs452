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
#include <lock.h>

#define PRINTF_MAX_LENGTH 128

// Task Creation
int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
void ExitProgram();

// Inter-task Communication
int Send(int Tid, void *msg, int msglen, void *reply, int replylen);
int Receive(int *tid, void *msg, int msglen);
int Reply(int tid, void *reply, int replylen);

// Interrupt Processing
int AwaitEvent(int eventid);
int AwaitSend(int eventid, char ch);
char AwaitRecv(int eventid);

// Name Server
int RegisterAs(char *name);
int WhoIs(char *name);

// Clock Server
int Delay(int ticks);
int Time();
int DelayUntil(int ticks);

// Input/Output
int Getc(int channel);
int Putc(int channel, char ch);
int PutStr(int channel, char *str);
int PutSizedStr(int channel, char *str, int size);
void Printf(int channel, char *fmt, ...);
int PrintfAt(int channel, int row, int col, char *fmt, ...);
int GetLogRow();
int Log(char *fmt, ...);
int IOidle(int channel);

// Performance Monitor using idle task.
void IAmIdleTask();
int IdlePercent();
void TurnMonitor(int onOrOff);

// Synchronization
void AcquireLock(Lock *lock);
void ReleaseLock(Lock *lock);

#endif
