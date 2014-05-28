/*
 *	task_queue.c
 */

#include <send_queue.h>

void sendQueue_init(SendQueue *q) {
    q->start = (Task*)0;
    q->end = (Task*)0;
    q->size = 0;
}

void sendQueue_push(SendQueue *q, Task *t) {
    q->size++;
    if (q->end != 0) {
        q->end->nextMessageTask = t;
    } else {
        /* This is the first task in queue. */
        q->start = t;
    }
    t->nextMessageTask = 0;
    q->end = t;
}

Task *sendQueue_pop(SendQueue *q) {
    if (sendQueue_empty(q)) {
        return (Task*)0;
    }
    Task *firstTask = q->start;
    q->size--;
    q->start = firstTask->nextMessageTask;
    if (sendQueue_empty(q)) {
        /* Poped last item in queue. */
        q->end = 0;
    }
    return firstTask;
}

int sendQueue_empty(SendQueue *q) {
    return q->size == 0;
}
