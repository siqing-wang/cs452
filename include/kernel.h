/*
 * kernel.h
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <bwio.h>

int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

#endif
