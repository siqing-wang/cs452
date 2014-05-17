/*
 *	queue.h
 *  	Implements a ring queue with fixed size.
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <task.h>

#define QUEUE_MAX_SIZE 128

typedef struct TaskQueue {
	Task *start;
	Task *end;
	int size;
} TaskQueue;


void queue_init(TaskQueue *q);
void queue_push(TaskQueue *q, Task *t);
Task *queue_pop(TaskQueue *q);
int queue_empty(TaskQueue *q);

#endif
