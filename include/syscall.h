/*
 * syscall.h
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define PRIORITY_LOW 3
#define PRIORITY_MED 7
#define PRIORITY_HIGH 11

/*
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
int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

#endif
