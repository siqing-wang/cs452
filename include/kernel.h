/*
 * kernel.h
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <context_switch.h>

int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

#endif
