/*
 * event_queue.h
 *      Implements a ring queue for tasks with fixed size.
 *
 *  eventQueue_init
 *      Initialize a event queue
 *  eventQueue_push
 *      Push to the back of the queue
 *  eventQueue_pop
 *      Pop the first element of the queue
 *  eventQueue_empty
 *      Return if queue is empty
 */

#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

#include <task.h>

typedef struct EventQueue {
    Task *start;
    Task *end;
    int size;
} EventQueue;

void eventQueue_init(EventQueue *q);
void eventQueue_push(EventQueue *q, Task *t);
Task *eventQueue_pop(EventQueue *q);
int eventQueue_empty(EventQueue *q);

#endif
