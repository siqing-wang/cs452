/*
 * event.c
 */

#include <event.h>
#include <scheduler.h>

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

void event_blockTask(SharedVariables* sharedVariables, Task* active, int eventId) {
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
        task->state = TASK_READY;
        scheduler_add(sharedVariables, task);
    }
}
