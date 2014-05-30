 /*
 * request.c
 */

#include <request.h>
#include <scheduler.h>
#include <send_queue.h>
#include <utils.h>

void storeRetValue(Task* task, int retVal);
int sendMessage(SharedVariables* sharedVariables, Task* active, Message *message);
void readMessage(SharedVariables* sharedVariables, Task* active, Message *message);
int replyMessage(SharedVariables* sharedVariables, Task* active, Message *message);

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
            storeRetValue(active, SUCCESS);
            break;
        case SYS_EXIT:
            task_exit(sharedVariables, active);
            storeRetValue(active, SUCCESS);
            return;
        case SYS_SEND:
            result = sendMessage(sharedVariables, active, request->message);
            storeRetValue(active, result);
            break;
        case SYS_RECV:
            readMessage(sharedVariables, active, request->message);
            /* No Error Code for this call (no return value) */
            storeRetValue(active, SUCCESS);
            break;
        case SYS_REPLY:
            result = replyMessage(sharedVariables, active, request->message);
            storeRetValue(active, result);
            break;
        default:
            /* Unrecognized syscall. */
            storeRetValue(active, ERR_UNKNOWN_SYSCALL);
            break;
    }
    /* Add current task back to scheduler. */
    if (active->state == TASK_ACTIVE) {
        active->state = TASK_READY;
        scheduler_add(sharedVariables, active);
    }
}

void storeRetValue(Task* task, int retVal) {
    /* Push value on to task's stack, and update stack pointer in task's structure. */
    task->sp = task->sp - 1;
    *(task->sp) = retVal;
}

int sendMessage(SharedVariables* sharedVariables, Task* active, Message *message) {
    Task* destTask = task_get(sharedVariables, message->destTid);
    if (destTask == 0) {
        return ERR_NOEXIST_TID;
    } else if (destTask->state == TASK_SEND_BLK) {
        /* A task in send block cannot receive msg. */
        return ERR_INCOMPLETE_SRR_TRANS;
    }

    /* Update msg. */
    message->srcTid = active->tid;
    active->message = message;

    if (destTask->state == TASK_RECV_BLK) {
        /* Destination is currently receive blocked. */
        memcopy((char*)(destTask->message), (char*)(message), sizeof(Message));
        destTask->state = TASK_READY;
        scheduler_add(sharedVariables, destTask);
        active->state = TASK_RPLY_BLK;
    } else {
        /* Receiver is not receive blocked. */
        sendQueue_push(destTask->send_queue, active);
        active->state = TASK_SEND_BLK;
    }

    return SUCCESS;
}

void readMessage(SharedVariables* sharedVariables, Task* active, Message *message) {
    if(sendQueue_empty(active->send_queue)) {
        active->state = TASK_RECV_BLK;
        active->message = message;
    } else {
        Task* srcTask = sendQueue_pop(active->send_queue);
        memcopy((char*)message, (char*)(srcTask->message), sizeof(Message));
        srcTask->state = TASK_RPLY_BLK;
    }
}

int replyMessage(SharedVariables* sharedVariables, Task* active, Message *message) {
    Task* destTask = task_get(sharedVariables, message->destTid);
    if (destTask == 0) {
        return ERR_NOEXIST_TID;
    } else if (destTask->state != TASK_RPLY_BLK) {
        /* Can only send a reply to a task in reply block state. */
        return ERR_NOT_REPLY_BLK;
    }

    memcopy((char*)(destTask->message), (char*)(message), sizeof(Message));
    destTask->state = TASK_READY;
    scheduler_add(sharedVariables, destTask);

    if (message->msglen > destTask->message->replylen) {
        return ERR_INSUFFICIENT_SPACE;
    }
    return SUCCESS;
}
