/*
 *	queue.c
 */

#include <task_queue.h>
#include <utils.h>

void queue_init(TaskQueue *q) {
	q->start = (Task*)0;
	q->end = (Task*)0;
	q->size = 0;
}

void queue_push(TaskQueue *q, Task *t) {
	q->size++;
	q->end->nextTaskInQueue = t;
	q->end = t;
}

Task *queue_pop(TaskQueue *q) {
	if (queue_empty(q)) {
		// TODO: queue do not handle -ve value. (currently pop(emptyQ) -> -1)
		return (Task*)0;
	}

	q->size--;
	Task *firstTask = q->start;
	q->start = firstTask->nextTaskInQueue;
	return firstTask;
}

int queue_empty(TaskQueue *q) {
	return q->size == 0;
}




