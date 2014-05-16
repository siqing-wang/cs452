/*
 * kernel.h
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <context_switch.h>
 
void initialize(Task** ts);
Task* schedule(Task** ts);

int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

#endif
