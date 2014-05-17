/*
 * task.h
 */

#ifndef __TASK_H__
#define __TASK_H__

#define TASK_MAX_NUM 128
#define STACK_SIZE 2048

typedef struct Task
{
    int *sp;

    int tid;
    int parent_tid;
    int priority;

    char *name;
    struct Task *nextTaskInQueue;
} Task;

void task_init();

Task* task_create(char* name, int parent_tid, int priority, void (*code));

#endif
