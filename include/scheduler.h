/*
 * scheduler.h
 */

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <task.h>
#include <shared_variable.h>

#define PRIORITY_MIN 0
#define PRIORITY_MAX 15

void scheduler_init(SharedVariables* sharedVariables);
void scheduler_add(SharedVariables* sharedVariables, Task* task);
Task* scheduler_getNextTask(SharedVariables* sharedVariables);

#endif
