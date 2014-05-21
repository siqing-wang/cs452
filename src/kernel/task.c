/*
* task.c
*/

#include <task.h>
#include <scheduler.h>

static Task tasks[TASK_MAX_NUM];
static int stack[STACK_SIZE * TASK_MAX_NUM];
static int nextTaskId;

void task_init() {
    nextTaskId = 0;
}

Task* task_create(int parent_tid, int priority, void (*code)) {
    if (nextTaskId >= TASK_MAX_NUM) {
        return (Task*)0;
    }
    if ((priority > PRIORITY_MAX) || (priority < PRIORITY_MIN)) {
        return (Task*)0;
    }
    Task* task = &tasks[nextTaskId];
    task->tid = nextTaskId;
    task->parent_tid = parent_tid;
    task->priority = priority;
    task->sp = stack + (nextTaskId + 1) * STACK_SIZE;
    task->sp = task->sp - 13;
    *(task->sp) = 1;
    *(task->sp + 1 ) = USER_MODE;
    *(task->sp + 2 ) = (int)code;
    task->nextTaskInQueue = 0;
    nextTaskId = nextTaskId + 1;
    return task;
}
