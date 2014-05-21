/*
 * scheduler.h
 */

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <task.h>

#define PRIORITY_MIN 0
#define PRIORITY_MAX 15


/*
 * scheduler_init
 *		Initialize priority queues in the scheduler
 * scheduler_add
 *		Add task to the corresponding priority queue.
 * scheduler_getNextTask
 *		Get next task from highest occupied priority queue.
 */
void scheduler_init();
void scheduler_add(Task* task);
Task* scheduler_getNextTask();

#endif
