/*
 * event.h
 *
 *  event_init
 *      Initialize event table and event queue in each event.
 *  event_addInterrupt
 *      Associate interrupt with an event.
 *  event_blockTask
 *      Block task on an event.
 *  eveny_unblockTask
 *      Unblock all tasks on events associated with given interrupt.
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <shared_variable.h>
#include <task.h>
#include <event_queue.h>

#define MAX_EVENTS_NUM          10

#define EVENT_TIMER             0
#define EVENT_TERMINAL_SEND     1
#define EVENT_TERMINAL_RECV     2
#define EVENT_TRAIN_SEND        3
#define EVENT_TRAIN_RECV        4


typedef struct Event
{
    /* 64 interrupts so need two int for 64 bits mask. */
    unsigned int interruptMask1;
    unsigned int interruptMask2;
    /* Tasks blocked on this event. */
    struct EventQueue *event_queue;
} Event;

void event_init(SharedVariables* sharedVariables);
void event_addInterrupt(SharedVariables* sharedVariables, int eventId, int interruptId);
void event_blockTask(SharedVariables* sharedVariables, Task* active, int eventId);
void event_unblockTask(SharedVariables* sharedVariables, int interruptId);

#endif
