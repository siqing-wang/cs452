/*
 * task.h
 */

#ifndef __TASK_H__
#define __TASK_H__

#define TASK_MAX_NUM 128
#define STACK_SIZE 2048
#define USER_MODE 0x50

typedef struct Task
{
    int *sp;

    int tid;
    int parent_tid;
    int priority;

    struct Task *nextTaskInQueue;
} Task;

void task_init();

Task* task_create(int parent_tid, int priority, void (*code));

#endif
