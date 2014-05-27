 /*
 * syscall.c
 */

#include <syscall.h>
#include <request.h>

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
int Send(int Tid, void *msg, int msglen, void *reply, int replylen) {
    Request request;
    request.syscall = SYS_SEND;
    return sendRequest(&request);
}

int Receive(int *tid, void *msg, int msglen) {
    Request request;
    request.syscall = SYS_RECV;
    return sendRequest(&request);
}

int Reply(int tid, void *reply, int replylen) {
    Request request;
    request.syscall = SYS_REPLY;
    return sendRequest(&request);
}

// Name Server
int RegisterAs(char *name) {
    Request request;
    request.syscall = SYS_REGAS;
    return sendRequest(&request);
}

int WhoIs(char *name) {
    Request request;
    request.syscall = SYS_WHOIS;
    return sendRequest(&request);
}

int sendRequest(Request* request) {
    /* Request's ptr is already in r0 */
    asm("swi");                         // Call kerent with request
    register int retVal asm ("r0");     // Return value from kernel at r0
    return retVal;
}
