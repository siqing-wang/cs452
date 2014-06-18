/*
 * io_queue.h
 *  Input/Output buffer. FIFO order.
 */

#ifndef __IO_QUEUE_H__
#define __IO_QUEUE_H__

#define IO_QUEUE_CAPACITY   20

typedef struct io_queue {
    char *buffer;
    int capacity;   // max capacity
    int start;
    int size;
} io_queue;

int ioQueue_init(io_queue *q, char* buffer, int size);
int ioQueue_empty(io_queue *q);
int ioQueue_full(io_queue *q);
void ioQueue_push(io_queue *q, char c);
char ioQueue_pop(io_queue *q);

#endif
