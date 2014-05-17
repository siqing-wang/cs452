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

Task* task_create(char* name, int parent_tid, int priority, void (*code)) {
    Task* task = &tasks[nextTaskId];
    task->tid = nextTaskId;
    task->parent_tid = parent_tid;
    task->priority = priority;
    task->name = name;
    task->sp = stack + (nextTaskId + 1) * STACK_SIZE -1;
    *(task->sp - 11) = code;
    *(task->sp - 12) = 0x10;
    task->nextTaskInQueue = 0;
    nextTaskId = nextTaskId + 1;
    return task;
}
