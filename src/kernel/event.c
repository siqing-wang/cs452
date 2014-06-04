/*
 * event.c
 */

#include <event.h>

void event_init(SharedVariables* sharedVariables) {
    Event *events = sharedVariables->events;
    EventQueue *event_queue = sharedVariables->event_queue;

    int i = 0;
    for (; i < MAX_EVENTS_NUM; i++) {
        (events + i)->interruptMask1 = 0;
        (events + i)->interruptMask1 = 0;
        (events + i)->event_queue = (EventQueue*)(event_queue + i);
        eventQueue_init((events + i)->event_queue);
    }
}

void event_addInterrupt(SharedVariables* sharedVariables, int eventId, int interruptId) {
    Event *event = (Event *)(sharedVariables->events + eventId);
    if (interruptId < 32) {
        event->interruptMask1 = (unsigned int)(event->interruptMask1) | (1 << interruptId);
    } else {
        event->interruptMask2 = (unsigned int)(event->interruptMask2) | (1 << (interruptId - 32));
    }
}

void event_hasInterrupt(SharedVariables* sharedVariables, int eventId, int interruptId) {
    Event *event = (Event *)(sharedVariables->events + eventId);
    if (interruptId < 32) {
        return (unsigned int)(event->interruptMask1) & (1 << interruptId);
    } else {
        return (unsigned int)(event->interruptMask2) & (1 << (interruptId - 32));
    }
}

void event_blockTask(SharedVariables* sharedVariables, int eventId, Task* active) {

}

void event_unblockTask(SharedVariables* sharedVariables, int eventId) {

}
