/*
 *  event.h
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <shared_variable.h>
#include <task.h>
#include <event_queue.h>

#define MAX_EVENTS_NUM 10

#define EVENT_TIMER 0

typedef struct Event
{
    unsigned int interruptMask1;
    unsigned int interruptMask2;
    struct EventQueue *event_queue;
} Event;

void event_init(SharedVariables* sharedVariables);
void event_addInterrupt(SharedVariables* sharedVariables, int eventId, int interruptId);
void event_hasInterrupt(SharedVariables* sharedVariables, int eventId, int interruptId);
void event_blockTask(SharedVariables* sharedVariables, int eventId, Task* active);
void event_unblockTask(SharedVariables* sharedVariables, int eventId);

#endif
