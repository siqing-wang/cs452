 /*
 * request.c
 */

#include <request.h>
#include <scheduler.h>
#include <send_queue.h>
#include <utils.h>

void request_handle(SharedVariables* sharedVariables, Task* active, Request *request) {
    switch(request->syscall) {
        Task *task;
        int result;
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
            task_exit(sharedVariables, active);
            storeRetValue(active, 0);
            return;
        case SYS_SEND:
            result = sendMessage(sharedVariables, active, request->message);
            storeRetValue(active, result);
        case SYS_WAITREPLY:
            result = readMessage(sharedVariables, active, request->message);
            storeRetValue(active, result);
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

int sendMessage(SharedVariables* sharedVariables, Task* active, Message *message) {
    Task* destTask = task_find(sharedVariables, message->destTid);
    if (destTask == 0) {
        return ERR_NOEXIST_TID;
    }

    message->srcTid = active->tid;
    active->message = message;
    sendQueue_push(destTask->send_queue, active);
    return 0;
}

int readMessage(SharedVariables* sharedVariables, Task* active, Message *message) {
    if(sendQueue_empty(active->send_queue)) {
        return NO_RECEIVED_MSG;
    }
    Task* srcTask = sendQueue_pop(active->send_queue);
    memcopy((char*)message, (char*)(srcTask->message), sizeof(srcTask->message));
    return HAS_RECEIVED_MSG;
}
