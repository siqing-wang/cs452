/*
 * event.c
 */

#include <event.h>
#include <scheduler.h>
#include <io.h>
#include <ts7200.h>
#include <request.h>
#include <io_queue.h>
#include <utils.h>

void event_init(SharedVariables* sharedVariables) {
    Event *events = sharedVariables->events;

    int i = 0;
    for (; i < NUM_EVENTS; i++) {
        /* Initialize blocked task for each event. */
        (events + i)->task = (Task *)0;
    }
}

void event_blockTask(SharedVariables* sharedVariables, Task* active, int eventId, char data) {
    if ((eventId == EVENT_TERMINAL_SEND) && (sharedVariables->com2TxReady)) {
        io_putdata(COM2, data);
        io_interrupt_enable(COM2, TIEN_MASK);
        sharedVariables->com2TxReady = 0;
        return;
    }

    if ((eventId == EVENT_TRAIN_SEND) && (sharedVariables->com1TxReady) && (sharedVariables->com1CtsReady)) {
        io_putdata(COM1, data);
        io_interrupt_enable(COM1, TIEN_MASK);
        sharedVariables->com1TxReady = 0;
        sharedVariables->com1CtsReady = 0;
        return;
    }

    Request *req = (Request *)(*(active->sp));
    if ((eventId == EVENT_TERMINAL_RECV) && (!ioQueue_empty(sharedVariables->com2IOQueue))) {
        req->data = ioQueue_pop(sharedVariables->com2IOQueue);
        return;
    }

    if ((eventId == EVENT_TRAIN_RECV) && (!ioQueue_empty(sharedVariables->com1IOQueue))) {
        req->data = ioQueue_pop(sharedVariables->com1IOQueue);
        return;
    }

    /* Find corresponding event. */
    Event *event = (Event *)(sharedVariables->events + eventId);

    /* Block task and push into blocked queue for the event. */
    active->state = TASK_EVENT_BLK;
    event->task = active;
}

void event_unblockTask(SharedVariables* sharedVariables, int eventId) {
    Event* event = (Event *)(sharedVariables->events + eventId);
    Task* task = event->task;

    if (task == (Task *)0) {
        return;
    }

    Request *req = (Request *)(*(task->sp));
    switch(eventId) {
        case EVENT_TERMINAL_SEND:
            io_putdata(COM2, req->data);
            io_interrupt_enable(COM2, TIEN_MASK);
            sharedVariables->com2TxReady = 0;
            break;
        case EVENT_TRAIN_SEND:
            io_putdata(COM1, req->data);
            io_interrupt_enable(COM1, TIEN_MASK);
            sharedVariables->com1TxReady = 0;
            sharedVariables->com1CtsReady = 0;
            break;
        case EVENT_TERMINAL_RECV:
            req->data = ioQueue_pop(sharedVariables->com2IOQueue);
            break;
        case EVENT_TRAIN_RECV:
            req->data = ioQueue_pop(sharedVariables->com1IOQueue);
            break;
    }

    task->state = TASK_READY;
    scheduler_add(sharedVariables, task);
    event->task = (Task *)0;
}
