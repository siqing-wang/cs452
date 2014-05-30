/*
 *  task.h
 *
 *  task_init
 *      Initialize static variable for task class
 *  task_create
 *      Get next available task pointer and initialize its fields
 *  task_exit
 *      Task has exited so recycle task descriptor for next use
 *  task_get
 *      Return corresponding Task* given tid or 0 if not found.
 */

#ifndef __TASK_H__
#define __TASK_H__

#include <shared_variable.h>
#include <message.h>

#define TASK_MAX_NUM        128
#define USER_STACK_LOW      0x300000
#define USER_STACK_HIGH     0x1300000
#define STACK_SIZE          ((USER_STACK_HIGH - USER_STACK_LOW) / TASK_MAX_NUM)
#define USER_MODE           0x50

#define TASK_UNINIT         0
#define TASK_ACTIVE         1
#define TASK_READY          2
#define TASK_ZOMBIE         3
#define TASK_SEND_BLK       4
#define TASK_RECV_BLK       5
#define TASK_RPLY_BLK       6

typedef struct Task
{
    int *sp;        // stack pointer of the task

    int tid;        // task id
    int parent_tid; // parent task id
    int priority;   // priority from 0 to PRIORITY_MAX
    int state;

    struct Task *next;  // next task in the same scheduler priority queue or free list, 0 is NULL

    /* Message for communication. */
    struct SendQueue *send_queue;   // keep track of tasks that want to send message to this task
    Message* message;               // Store this task's own message
    struct Task *nextMessageTask;   // next task contains message which has the same dest as this task
} Task;


void task_init(SharedVariables* sharedVariables);
Task* task_create(SharedVariables* sharedVariables, int parent_tid, int priority, void (*code));
void task_exit(SharedVariables* sharedVariables, Task* task);
Task* task_get(SharedVariables* sharedVariables, int tid);

#endif
