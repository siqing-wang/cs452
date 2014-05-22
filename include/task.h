/*
 * task.h
 */

#ifndef __TASK_H__
#define __TASK_H__

#include <shared_variable.h>

#define TASK_MAX_NUM 128
#define STACK_SIZE 2048
#define USER_MODE 0x50

typedef struct Task
{
    int *sp;		// stack pointer of the task

    int tid;		// task id
    int parent_tid;	// parent task id
    int priority;	// priority from 0 to PRIORITY_MAX

    struct Task *nextTaskInQueue;	// next task in the same scheduler priority queue, 0 is NULL
} Task;

/*
 * task_init
 *		Initialize static variable for task class
 * task_create
 *		Get next available task pointer and initialize its fields
 */
void task_init(SharedVariables* sharedVariables);
Task* task_create(SharedVariables* sharedVariables, int parent_tid, int priority, void (*code));

#endif
