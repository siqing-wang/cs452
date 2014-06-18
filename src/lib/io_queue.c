#include <io_queue.h>
#include <utils.h>

void ioQueue_init(IOQueue *q) {
    q->start = 0;
    q->size = 0;
}

int ioQueue_empty(IOQueue *q) {
    return (q->size == 0);
}

int ioQueue_full(IOQueue *q) {
    return (q->size >= IO_QUEUE_CAPACITY);
}

void ioQueue_push(IOQueue *q, char c) {
    if (ioQueue_full(q)) {
        warning("io_queue: try to PUSH into a FULL queue.");
        return;
    }

    q->buffer[(q->start + q->size) % IO_QUEUE_CAPACITY] = c;
    q->size ++;
}

char ioQueue_pop(IOQueue *q) {
    if (ioQueue_empty(q)) {
        warning("io_queue: try to POP from an EMPTY queue.");
        return -1;
    }

    q->size--;
    char c = q->buffer[q->start];
    q->start = (q->start + 1) % IO_QUEUE_CAPACITY;
    return c;
}
