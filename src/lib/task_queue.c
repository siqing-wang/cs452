/*
 *	queue.c
 */

#include <task_queue.h>

void queue_init(TaskQueue *q) {
	q->start = (Task*)0;
	q->end = (Task*)0;
	q->size = 0;
}

void queue_push(TaskQueue *q, Task *t) {
	q->size++;
	if (q->end != 0) {
		q->end->nextTaskInQueue = t;
	} else {
		/* This is the first task in queue. */
		q->start = t;
	}
	t->nextTaskInQueue = 0;
	q->end = t;
}

Task *queue_pop(TaskQueue *q) {
	if (queue_empty(q)) {
		return (Task*)0;
	}
	Task *firstTask = q->start;
	q->size--;
	q->start = firstTask->nextTaskInQueue;
	if (queue_empty(q)) {
		/* Poped last item in queue. */
		q->end = 0;
	}
	return firstTask;
}

int queue_empty(TaskQueue *q) {
	return q->size == 0;
}
