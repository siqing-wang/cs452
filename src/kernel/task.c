/*
* task.c
*/

#include <task.h>

static Task tasks[100];
static int nextTaskId;

void initTasks(Task** ts) {
    nextTaskId = 0;
    // *ts = tasks;
}

void createTask(char* name) {
    Task* task = &tasks[nextTaskId];
    task->name = name;
    task->sp = RAM_TOP;
    task->spsr = 0x10;
    nextTaskId = nextTaskId + 1;
}
