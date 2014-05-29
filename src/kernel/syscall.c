 /*
 * syscall.c
 */

#include <syscall.h>
#include <request.h>
#include <message.h>
#include <utils.h>

// Task Creation
int Create(int priority, void (*code)()) {
    Request request;                    // Create request structure and store required fields.
    request.syscall = SYS_CREATE;
    request.priority = priority;
    request.code = code;
    return sendRequest(&request);
}

int MyTid() {
    Request request;
    request.syscall = SYS_MYTID;
    return sendRequest(&request);
}

int MyParentTid() {
    Request request;
    request.syscall = SYS_MYPID;
    return sendRequest(&request);
}

void Pass() {
    Request request;
    request.syscall = SYS_PASS;
    sendRequest(&request);
}

void Exit() {
    Request request;
    request.syscall = SYS_EXIT;
    sendRequest(&request);
}

// Inter-task Communication
int Send(int tid, void *msg, int msglen, void *reply, int replylen) {
    if (tid <= 0) {
        return ERR_INVALID_TID;
    }

    Message message;
    message.destTid = tid;
    message.msglen = msglen;
    message.replylen = replylen;
    // memcopy(message.msg, msg, msglen);
    message.msg = msg;

    Request request;
    request.syscall = SYS_SEND;
    request.message = &message;
    int result = sendRequest(&request);
    if (result < 0) {
        // Send failed, do not wait for reply.
        return result;
    }
    memcopy(reply, message.msg, replylen);
    return message.msglen;
}

int Receive(int *tid, void *msg, int msglen) {
    Message message;

    Request request;
    request.syscall = SYS_RECV;
    request.message = &message;

    sendRequest(&request);
    memcopy(msg, message.msg, msglen);
    *tid = message.srcTid;
    return message.msglen;
}

int Reply(int tid, void *reply, int replylen) {
    if (tid <= 0) {
        return ERR_INVALID_TID;
    }

    Message message;
    message.destTid = tid;
    message.msglen = replylen;
    // memcopy(message.msg, msg, msglen);
    message.msg = reply;

    Request request;
    request.syscall = SYS_REPLY;
    request.message = &message;
    int result = sendRequest(&request);
    if (result < 0) {
        // Syscall failed
        return result;
    }

    return SUCCESS;
}

// Name Server
int RegisterAs(char *name) {
    int msglen = 0;
    while (*(name+msglen) != '\0') {
        msglen++;
    }

    Message message;
    message.destTid = NAMESERVER_TID;
    message.msglen = msglen;
    message.replylen = sizeof(int);
    message.msg = name;

    Request request;
    request.syscall = SYS_SEND;
    request.message = &message;
    int result = sendRequest(&request);
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    int *status = (int*) message.msg;
    if (*status != NAMESERVER_SUCCESS) {
        return ERR_NOT_NAMESERVER;
    }
    return SUCCESS;
}

int WhoIs(char *name) {
    int msglen = 0;
    while (*(name+msglen) != '\0') {
        msglen++;
    }

    Message message;
    message.destTid = NAMESERVER_TID;
    message.msglen = msglen;
    message.replylen = sizeof(int);
    message.msg = name;

    Request request;
    request.syscall = SYS_SEND;
    request.message = &message;
    int result = sendRequest(&request);
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    int *tid = (int*)message.msg;
    if (*tid <= 0) {
        return ERR_NOT_NAMESERVER;
    }
    return (int)*tid;
}

int sendRequest(Request* request) {
    /* Request's ptr is already in r0 */
    asm("swi");                         // Call kerent with request
    register int retVal asm ("r0");     // Return value from kernel at r0
    return retVal;
}
