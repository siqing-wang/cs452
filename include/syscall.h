/*
 * syscall.h
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define PRIORITY_LOW 3
#define PRIORITY_MED 7
#define PRIORITY_HIGH 11
 
int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

#endif
