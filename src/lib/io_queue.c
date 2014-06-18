#include <io_queue.h>
#include <utils.h>

int ioQueue_init(io_queue *q, char* buffer, int size) {
    q->buffer = buffer;
    q->capacity = size;
    q->start = 0;
    q->size = 0;
}

int ioQueue_empty(io_queue *q) {
    return q->size == 0;
}

int ioQueue_full(io_queue *q) {
    return q->size >= q->capacity;
}

void ioQueue_push(io_queue *q, char c) {
    if (ioQueue_full(queue)) {
        warning("io_queue: try to PUSH into a FULL queue.")
        return;
    }

    int index = (q->start + q->size) % q->capacity;
    q->size++;
    *(q->buffer + index) = c;
}

char ioQueue_pop(io_queue *q) {
    if (ioQueue_empty(channel)) {
        warning("io_queue: try to POP from an EMPTY queue.")
        return -1;
    }

    q->size--;
    char c = (char) q->buffer[q->start];
    q->start = (q->start + 1) % q->capacity;
    return c;
}
