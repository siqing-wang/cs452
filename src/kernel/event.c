/*
 * event.c
 */

#include <event.h>
#include <scheduler.h>

int event_hasInterrupt(Event* event, int interruptId);

void event_init(SharedVariables* sharedVariables) {
    Event *events = sharedVariables->events;
    EventQueue *event_queue = sharedVariables->event_queue;

    int i = 0;
    for (; i < MAX_EVENTS_NUM; i++) {
        /* Initialize blocked queue and interrupt mask for each event. */
        (events + i)->interruptMask1 = 0;
        (events + i)->interruptMask2 = 0;
        (events + i)->event_queue = (EventQueue*)(event_queue + i);
        eventQueue_init((events + i)->event_queue);
    }
}

void event_addInterrupt(SharedVariables* sharedVariables, int eventId, int interruptId) {
    /* Find corresponding event. */
    Event *event = (Event *)(sharedVariables->events + eventId);

    /* Update corresponding interrupt mask bit. */
    if (interruptId < 32) {
        event->interruptMask1 = (unsigned int)(event->interruptMask1) | (1 << interruptId);
    } else {
        event->interruptMask2 = (unsigned int)(event->interruptMask2) | (1 << (interruptId - 32));
    }
}

void event_blockTask(SharedVariables* sharedVariables, Task* active, int eventId) {
    /* Find corresponding event. */
    Event *event = (Event *)(sharedVariables->events + eventId);

    /* Block task and push into blocked queue for the event. */
    active->state = TASK_EVENT_BLK;
    eventQueue_push(event->event_queue, active);
}

void event_unblockTask(SharedVariables* sharedVariables, int interruptId) {
    int i = 0;
    Event* event;
    /* Go through all events. */
    for(;i<MAX_EVENTS_NUM;i++) {
        event = (Event *)(sharedVariables->events + i);
        if (event_hasInterrupt(event, interruptId)) {
            /* Event is associated with given interrupt. */
            EventQueue *queue = event->event_queue;
            Task* task;
            while(!eventQueue_empty(queue)) {
                /* Unblock all tasks waiting for the event. */
                task = eventQueue_pop(queue);
                task->state = TASK_READY;
                scheduler_add(sharedVariables, task);
            }
        }
    }
}

int event_hasInterrupt(Event* event, int interruptId) {
    /* Check is the event is associated with given interrupt. */
    if (interruptId < 32) {
        return (unsigned int)(event->interruptMask1) & (1 << interruptId);
    } else {
        return (unsigned int)(event->interruptMask2) & (1 << (interruptId - 32));
    }
}
