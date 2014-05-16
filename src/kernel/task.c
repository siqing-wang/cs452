/*
* task.c
*/

#include <task.h>

static Task tasks[100];
static int nextTaskId;

void task_init() {
    nextTaskId = 0;
    // *ts = tasks;
}

Task* task_create(char* name) {
    Task* task = &tasks[nextTaskId];
    // task->name = name;
    // task->sp = RAM_TOP;
    // task->spsr = 0x10;
    nextTaskId = nextTaskId + 1;
    return task;
}
