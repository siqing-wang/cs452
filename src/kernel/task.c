/*
* task.c
*/

#include <task.h>
#include <scheduler.h>
#include <task_queue.h>
#include <send_queue.h>

void task_init(SharedVariables* sharedVariables) {
    Task *tasks = sharedVariables->tasks;
    SendQueue *send_queues = sharedVariables->send_queues;
    TaskQueue *free_list = sharedVariables->free_list;
    taskQueue_init(free_list);

    int i = 0;
    for (; i < TASK_MAX_NUM; i++) {
        (tasks + i)->tid = i;                       // assign initial tid = posn in array
        (tasks + i)->state = TASK_UNINIT;
        int *addr = (int *)(USER_STACK_LOW + (i + 1) * STACK_SIZE);     // One posn below beginning of task stack
                                                                        // because it is full stack (i.e. point to next full spot)
        (tasks + i)->sp = addr;                             // sp need to be a ptr to an address

        (tasks + i)->send_queue = (SendQueue*)(send_queues + i);
        sendQueue_init((tasks + i)->send_queue);            // Initialize sendQueue
        (tasks + i)->message = 0;

        taskQueue_push(free_list, tasks + i);
    }
}

Task* task_create(SharedVariables* sharedVariables, int parent_tid, int priority, void (*code)) {

    /* Get free_list from shared variables. */
    TaskQueue* free_list = sharedVariables->free_list;

    if ((priority > PRIORITY_MAX) || (priority < PRIORITY_MIN)) {
        /* Invalid priority. */
        return (Task*)0;
    }

    if (taskQueue_empty(free_list)) {
        /* Currently no free task descriptor. */
        return (Task*)0;
    }


    Task* task = taskQueue_pop(free_list);          // Get next avaiable task ptr.

    task->parent_tid = parent_tid;
    task->priority = priority;
    task->state = TASK_READY;
    task->sp = task->sp - 14;                   // Move up 14 to store 14 registers
    *(task->sp) = 1;                            // return value = 1
    *(task->sp + 1 ) = USER_MODE;               // spsr = USER_MODE
    *(task->sp + 2 ) = (int)(sharedVariables->loadOffset) + (int)code;      // pc = code
    task->next = 0;                  // no task after it in scheduler queue because its currently the last

    return task;
}

void task_exit(SharedVariables* sharedVariables, Task* task) {
    task->tid += TASK_MAX_NUM;                      // Update tid to avoid collision
    task->state = TASK_ZOMBIE;
    taskQueue_push(sharedVariables->free_list, task);   // Put task back to free_list
}

Task* task_get(SharedVariables* sharedVariables, int tid) {
    int index = tid % TASK_MAX_NUM;
    Task* task = (Task*)(sharedVariables->tasks + index);

    switch(task->state) {
        case TASK_UNINIT:
        case TASK_ZOMBIE:
            return (Task*)0;
    }
    if (task->tid != tid) {
        return (Task*)0;
    }

    return task;
}
