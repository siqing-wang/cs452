/*
 *  sned_queue.h
 *      Implements a ring queue for tasks with fixed size.
 *
 *  sendQueue_init
 *      Initialize a send queue
 *  sendQueue_push
 *      Push to the back of the queue
 *  sendQueue_pop
 *      Pop the first element of the queue
 *  sendQueue_empty
 *      Return if queue is empty
 */

#ifndef __SEND_QUEUE_H__
#define __SEND_QUEUE_H__

#include <task.h>

typedef struct SendQueue {
    Task *start;
    Task *end;
    int size;
} SendQueue;

void sendQueue_init(SendQueue *q);
void sendQueue_push(SendQueue *q, Task *t);
Task *sendQueue_pop(SendQueue *q);
int sendQueue_empty(SendQueue *q);

#endif
