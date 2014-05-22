/*
 * shared_variable.h
 */

#ifndef __SHARED_VARIABLE_H__
#define __SHARED_VARIABLE_H__

struct Task;
struct TaskQueue;

typedef struct SharedVariables
{
	struct TaskQueue* task_queues;
    int* highestOccupiedQueue;
    struct Task* tasks;
    int* nextTaskId;
    int loadOffset;
} SharedVariables;

#endif
