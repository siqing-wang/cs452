/*
 *  task_queue.h
 *      Implements a ring queue for tasks with fixed size.
 *
 *  taskQueue_init
 *      Initialize a task queue
 *  taskQueue_push
 *      Push to the back of the queue
 *  taskQueue_pop
 *      Pop the first element of the queue
 *  taskQueue_empty
 *      Return if queue is empty
 */

#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#include <task.h>

typedef struct TaskQueue {
    Task *start;
    Task *end;
    int size;
} TaskQueue;


void taskQueue_init(TaskQueue *q);
void taskQueue_push(TaskQueue *q, Task *t);
Task *taskQueue_pop(TaskQueue *q);
int taskQueue_empty(TaskQueue *q);

#endif
