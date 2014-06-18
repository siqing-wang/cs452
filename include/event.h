/*
 * event.h
 *
 *  event_init
 *      Initialize event table and event queue in each event.
 *  event_blockTask
 *      Block task on an event.
 *  eveny_unblockTask
 *      Unblock all tasks on an events.
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <shared_variable.h>
#include <task.h>
#include <event_queue.h>

#define NUM_EVENTS              7

#define EVENT_TIMER             0
#define EVENT_TERMINAL_SEND     1
#define EVENT_TERMINAL_RECV     2
#define EVENT_TERMINAL_CTRL     3
#define EVENT_TRAIN_SEND        4
#define EVENT_TRAIN_RECV        5
#define EVENT_TRAIN_CTRL        6


typedef struct Event
{
    /* Tasks blocked on this event. */
    struct EventQueue *event_queue;
} Event;

void event_init(SharedVariables* sharedVariables);
void event_blockTask(SharedVariables* sharedVariables, Task* active, int eventId);
void event_unblockTask(SharedVariables* sharedVariables, int eventId);

#endif
