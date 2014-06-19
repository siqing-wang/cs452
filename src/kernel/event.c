/*
 * event.c
 */

#include <event.h>
#include <scheduler.h>
#include <io.h>
#include <ts7200.h>
#include <request.h>
#include <utils.h>

void event_init(SharedVariables* sharedVariables) {
    Event *events = sharedVariables->events;
    EventQueue *event_queue = sharedVariables->event_queue;

    int i = 0;
    for (; i < NUM_EVENTS; i++) {
        /* Initialize blocked queue for each event. */
        (events + i)->event_queue = (EventQueue*)(event_queue + i);
        eventQueue_init((events + i)->event_queue);
    }
}

void event_blockTask(SharedVariables* sharedVariables, Task* active, int eventId, char data) {
    if ((eventId == EVENT_TERMINAL_SEND) && (sharedVariables->com2TxReady)) {
        io_putdata(COM2, data);
        io_interrupt_enable(COM2, TIEN_MASK);
        sharedVariables->com2TxReady = 0;
        return;
    }
    if ((eventId == EVENT_TRAIN_SEND) && (sharedVariables->com1TxReady)) {
        io_putdata(COM1, data);
        io_interrupt_enable(COM1, TIEN_MASK);
        sharedVariables->com1TxReady = 0;
        return;
    }

    /* Find corresponding event. */
    Event *event = (Event *)(sharedVariables->events + eventId);

    /* Block task and push into blocked queue for the event. */
    active->state = TASK_EVENT_BLK;
    eventQueue_push(event->event_queue, active);
}

void event_unblockTask(SharedVariables* sharedVariables, int eventId) {
    Event* event = (Event *)(sharedVariables->events + eventId);
    /* Event is associated with given interrupt. */
    EventQueue *queue = event->event_queue;
    Task* task;
    while(!eventQueue_empty(queue)) {
        /* Unblock all tasks waiting for the event. */
        task = eventQueue_pop(queue);
        Request *req = (Request *)(*(task->sp));

        switch(eventId) {
            char ch;
            case EVENT_TERMINAL_SEND:
                io_putdata(COM2, req->data);
                io_interrupt_enable(COM2, TIEN_MASK);
                sharedVariables->com2TxReady = 0;
                break;
            case EVENT_TRAIN_SEND:
                io_putdata(COM1, req->data);
                io_interrupt_enable(COM1, TIEN_MASK);
                sharedVariables->com1TxReady = 0;
                break;
            case EVENT_TERMINAL_RECV:
                ch = io_getdata(COM2);
                req->data = ch;
                break;
            case EVENT_TRAIN_RECV:
                ch = io_getdata(COM1);
                req->data = ch;
                break;
        }

        task->state = TASK_READY;
        scheduler_add(sharedVariables, task);
    }
}
