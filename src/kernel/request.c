 /*
 * request.c
 */

#include <request.h>
#include <scheduler.h>

void request_handle(SharedVariables* sharedVariables, Task* active, Request *request) {
    switch(request->syscall) {
        Task *task;
        case SYS_CREATE:
            task = task_create(sharedVariables, active->tid, request->priority, request->code);
            if (task == 0) {
                /* Task creation failed. */
                storeRetValue(active, ERR_CREATE_TASK_FAIL);
            }
            else {
                scheduler_add(sharedVariables, task);       // Add the created task to scheduler
                storeRetValue(active, task->tid);
            }
            break;
        case SYS_MYTID:
            storeRetValue(active, active->tid);
            break;
        case SYS_MYPID:
            storeRetValue(active, active->parent_tid);
            break;
        case SYS_PASS:
            storeRetValue(active, 0);
            break;
        case SYS_EXIT:
            storeRetValue(active, 0);
            return;
        default:
            /* Unrecognized syscall. */
            storeRetValue(active, ERR_UNKNOWN_SYSCALL);
            break;
    }
    /* Add current task back to scheduler. */
    scheduler_add(sharedVariables, active);
}

void storeRetValue(Task* task, int retVal) {
    /* Push value on to task's stack, and update stack pointer in task's structure. */
    task->sp = task->sp - 1;
    *(task->sp) = retVal;
}
