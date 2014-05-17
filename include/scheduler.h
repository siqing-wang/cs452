/*
 * scheduler.h
 */

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <task.h>

#define PRIORITY_MAX 15

void scheduler_init();
void scheduler_add(Task* task);
Task* scheduler_getNextTask();

#endif
