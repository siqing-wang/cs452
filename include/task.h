/*
 * task.h
 */

#ifndef __TASK_H__
#define __TASK_H__

typedef struct Task
{
    unsigned int sp;
    unsigned int spsr;
    char *name;
    struct Task *nextTaskInQueue;
} Task;

void task_init();

Task* task_create(char* name);

#endif
