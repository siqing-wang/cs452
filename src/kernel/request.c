 /*
 * request.c
 */

#include <request.h>
#include <scheduler.h>

void request_handle(Task* active, Request *request) {
    switch(request->syscall) {
        Task *task;
        case SYS_CREATE:
            task = task_create(active->tid, request->priority, request->code);
            scheduler_add(task);
            storeRetValue(active, task->tid);
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
            storeRetValue(active, ERR_UNKNOWN_SYSCALL);
            break;
    }
    scheduler_add(active);
}

void storeRetValue(Task* task, int retVal) {
    task->sp = task->sp - 1;
    *(task->sp) = retVal;
}
