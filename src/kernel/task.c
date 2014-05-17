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

Task* task_create(char* name, int pid, int p) {
    Task* task = &tasks[nextTaskId];
    task->tid = nextTaskId;
    task->parent_tid = pid;
    task->priority = p;
    task->name = name;
    task->sp = stack + (nextTaskId + 1) * STACK_SIZE -1;
    task->spsr = 0x10;
    task->nextTaskInQueue = 0;
    nextTaskId = nextTaskId + 1;
    return task;
}
