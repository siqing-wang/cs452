/*
* task.c
*/

#include <task.h>
#include <scheduler.h>
#include <task_queue.h>

void task_init(SharedVariables* sharedVariables) {
    Task *tasks = sharedVariables->tasks;
    TaskQueue *free_list = sharedVariables->free_list;
    queue_init(free_list);

    int i = 0;
    for (; i < TASK_MAX_NUM; i++) {
        (tasks + i)->tid = i;                       // assign initial tid = posn in array
        int *addr = (int *)(USER_STACK_LOW + (i + 1) * STACK_SIZE);     // One posn below beginning of task stack
                                                                        // because it is full stack (i.e. point to next full spot)
        (tasks + i)->sp = addr;                            // sp need to be a ptr to an address
        queue_push(free_list, tasks + i);
    }
}

Task* task_create(SharedVariables* sharedVariables, int parent_tid, int priority, void (*code)) {

    /* Get free_list from shared variables. */
    TaskQueue* free_list = sharedVariables->free_list;

    if ((priority > PRIORITY_MAX) || (priority < PRIORITY_MIN)) {
        /* Invalid priority. */
        return (Task*)0;
    }

    if (queue_empty(free_list)) {
        /* Currently no free task descriptor. */
        return (Task*)0;
    }


    Task* task = queue_pop(free_list);          // Get next avaiable task ptr.

    task->parent_tid = parent_tid;
    task->priority = priority;
    task->sp = task->sp - 13;                   // Move up 13 to store 13 registers
    *(task->sp) = 1;                            // return value = 1
    *(task->sp + 1 ) = USER_MODE;               // spsr = USER_MODE
    *(task->sp + 2 ) = (int)(sharedVariables->loadOffset) + (int)code;      // pc = code
    task->next = 0;                  // no task after it in scheduler queue because its currently the last

    return task;
}

void task_exit(SharedVariables* sharedVariables, Task* task) {
    task->tid += TASK_MAX_NUM;                      // Update tid to avoid collision
    queue_push(sharedVariables->free_list, task);   // Put task back to free_list
}
