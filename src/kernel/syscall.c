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
    message.type = MSG_STRING;
    message.msglen = msglen;
    message.replylen = replylen;
    memcopy(message.msg, msg, msglen);

    Request request;
    request.syscall = SYS_SEND;
    request.message = &message;
    int result = sendRequest(&request);
    if (result < 0) {
        // Syscall failed
        return result;
    }

    int received = NO_RECEIVED_MSG;
    for(;;) {
        Request request;
        request.syscall = SYS_TRY_RECV;
        request.message = &message;
        received = sendRequest(&request);
        if (received == HAS_RECEIVED_MSG) {
            // Received reply
            break;
        }
    }

    memcopy(reply, message.msg, replylen);
    return message.msglen;
}

int Receive(int *tid, void *msg, int msglen) {
    Message message;

    int received = NO_RECEIVED_MSG;
    for(;;) {
        Request request;
        request.syscall = SYS_TRY_RECV;
        request.message = &message;
        received = sendRequest(&request);
        if (received == HAS_RECEIVED_MSG) {
            // Received reply
            break;
        }
    }

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
    message.type = MSG_STRING;
    message.msglen = replylen;
    memcopy(message.msg, reply, replylen);

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
    message.type = MSG_REGAS;
    message.msglen = msglen;
    memcopy(message.msg, name, msglen);

    Request request;
    request.syscall = SYS_SEND;
    request.message = &message;

    int result = sendRequest(&request);
    if (result < 0) {
        // Syscall failed
        return result;
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
    message.type = MSG_WHOIS;
    message.msglen = msglen;
    memcopy(message.msg, name, msglen);

    Request request;
    request.syscall = SYS_SEND;
    request.message = &message;

    int result = sendRequest(&request);
    if (result < 0) {
        // Syscall failed
        return result;
    }

    int received = NO_RECEIVED_MSG;
    for(;;) {
        Request request;
        request.syscall = SYS_TRY_RECV;
        request.message = &message;
        received = sendRequest(&request);
        if (received == HAS_RECEIVED_MSG) {
            // Received reply
            break;
        }
    }

    if (message.type != MSG_TID) {
        return ERR_INVALID_REPLY;
    }

    int *tid = message.msg;
    return *tid;
}

int sendRequest(Request* request) {
    /* Request's ptr is already in r0 */
    asm("swi");                         // Call kerent with request
    register int retVal asm ("r0");     // Return value from kernel at r0
    return retVal;
}
