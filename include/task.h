/*
 * task.h
 */

#ifndef __TASK_H__
#define __TASK_H__

#include <memory.h>

typedef struct Task
{
    unsigned int sp;
    unsigned int spsr;

    char* name;
} Task;

void initTasks(Task** ts);

void createTask(char* name);

#endif
