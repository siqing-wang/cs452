/*
* task.c
*/

#include <task.h>
#include <scheduler.h>

static Task tasks[TASK_MAX_NUM];                // pre-alloc spaces for all tasks
static int stack[STACK_SIZE * TASK_MAX_NUM];    // pre-alloc spaces for all tasks' stacks
static int nextTaskId;                          // keep track of next available task slot

void task_init() {
    nextTaskId = 0;
}

Task* task_create(int parent_tid, int priority, void (*code)) {
    if (nextTaskId >= TASK_MAX_NUM) {
        /* Created too many tasks. */
        return (Task*)0;
    }
    if ((priority > PRIORITY_MAX) || (priority < PRIORITY_MIN)) {
        /* Invalid priority. */
        return (Task*)0;
    }
    Task* task = &tasks[nextTaskId];        // Get next avaiable task ptr.
    task->tid = nextTaskId;
    task->parent_tid = parent_tid;
    task->priority = priority;
    task->sp = stack + (nextTaskId + 1) * STACK_SIZE;   // Next task's beginning in stack
    task->sp = task->sp - 13;                           // Move up 13 to store 13 registers
    *(task->sp) = 1;                        // return value = 1
    *(task->sp + 1 ) = USER_MODE;           // spsr = USER_MODE
    *(task->sp + 2 ) = (int)code;           // pc = code
    task->nextTaskInQueue = 0;              // no task after it in scheduler queue because its currently the last
    nextTaskId = nextTaskId + 1;
    return task;
}
