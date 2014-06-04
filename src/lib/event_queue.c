/*
 *	event_queue.c
 */

#include <event_queue.h>

void eventQueue_init(EventQueue *q) {
    q->start = (Task*)0;
    q->end = (Task*)0;
    q->size = 0;
}

void eventQueue_push(EventQueue *q, Task *t) {
    q->size++;
    if (q->end != 0) {
        q->end->nextBlockedTask = t;
    } else {
        /* This is the first task in queue. */
        q->start = t;
    }
    t->nextBlockedTask = 0;
    q->end = t;
}

Task *eventQueue_pop(EventQueue *q) {
    if (eventQueue_empty(q)) {
        return (Task*)0;
    }
    Task *firstTask = q->start;
    q->size--;
    q->start = firstTask->nextBlockedTask;
    if (eventQueue_empty(q)) {
        /* Poped last item in queue. */
        q->end = 0;
    }
    return firstTask;
}

int eventQueue_empty(EventQueue *q) {
    return q->size == 0;
}
