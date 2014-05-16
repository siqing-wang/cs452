/*
 * kernel.h
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <context_switch.h>


#define RAM_BASE 0x00000000
#define RAM_TOP 0x01ffffff
#define ROM_BASE 0x60000000
#define RAM_TOP 0x607fffff
#define DEVICE_REGISTERS_BASE 0x80800000
#define DEVICE_REGISTERS_TOP 0x808fffff
 
int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();

#endif
