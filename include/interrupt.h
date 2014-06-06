/*
 * interrupt.h
 *
 *  interrupt_init
 *      Initialize and enable hardware interrup.
 *  interrupt_handle
 *      Handle interrupt.
 *  interrupt_reset
 *      Disable hardware interrupt, set it back to original state.
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <shared_variable.h>
#include <task.h>

/* timer interrupt's bit location in VIC. EP9301 User's Guide Chap 5.1.1 */
#define INTERRUPT_TIMER 51

void interrupt_init(SharedVariables* sharedVariables);
void interrupt_handle(SharedVariables* sharedVariables, Task* active);
void interrupt_reset();

#endif
