/*
 * shared_variable.h
 *    This is used to pass in variables alloced in kernel stack to kernel components.
 *    This is to avoid having any .bss.
 */

#ifndef __SHARED_VARIABLE_H__
#define __SHARED_VARIABLE_H__

struct Task;
struct TaskQueue;
struct SendQueue;

typedef struct SharedVariables
{
    /* Scheduler */
    struct TaskQueue* task_queues;
    int* highestOccupiedQueue;
    /* Task */
    struct Task* tasks;
    struct SendQueue* send_queues;  // send_queue for each task
    struct TaskQueue* free_list;    // queue of free task descriptors
    /* Event */
    struct Event* events;           // event table
    /* Global */
    int loadOffset;                 // stack base (0x00218000) it should be added
                                    // to any memory addresses.
    int com1TxReady;
    int com2TxReady;
} SharedVariables;

#endif
