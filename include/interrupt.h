/*
 *  interrupt.h
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <shared_variable.h>
#include <task.h>

#define INTERRUPT_TIMER 51

void interrupt_init();
void interrupt_handle(SharedVariables* sharedVariables, Task* active);

#endif
