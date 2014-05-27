/*
 *	task_queue.c
 */

#include <task_queue.h>

void taskQueue_init(TaskQueue *q) {
    q->start = (Task*)0;
    q->end = (Task*)0;
    q->size = 0;
}

void taskQueue_push(TaskQueue *q, Task *t) {
    q->size++;
    if (q->end != 0) {
        q->end->next = t;
    } else {
        /* This is the first task in queue. */
        q->start = t;
    }
    t->next = 0;
    q->end = t;
}

Task *taskQueue_pop(TaskQueue *q) {
    if (taskQueue_empty(q)) {
        return (Task*)0;
    }
    Task *firstTask = q->start;
    q->size--;
    q->start = firstTask->next;
    if (taskQueue_empty(q)) {
        /* Poped last item in queue. */
        q->end = 0;
    }
    return firstTask;
}

int taskQueue_empty(TaskQueue *q) {
    return q->size == 0;
}
