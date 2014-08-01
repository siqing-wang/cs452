/*
 * io_queue.h
 *  Input/Output buffer. FIFO order.
 */

#ifndef __IO_QUEUE_H__
#define __IO_QUEUE_H__

#define IO_QUEUE_CAPACITY   512

typedef struct IOQueue {
    char buffer[IO_QUEUE_CAPACITY];
    int start;
    int size;
} IOQueue;

void ioQueue_init(IOQueue *q);
int ioQueue_empty(IOQueue *q);
int ioQueue_full(IOQueue *q);
void ioQueue_push(IOQueue *q, char c);
char ioQueue_pop(IOQueue *q);

#endif
