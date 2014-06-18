 /*
 * syscall.c
 */

#include <syscall.h>
#include <request.h>
#include <message.h>
#include <utils.h>
#include <nameserver.h>
#include <clockserver.h>
#include <ioserver.h>

/* Internal helper. */

int sendRequest(Request* request);

/* Task Creation */

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

/* Inter-task Communication */

int Send(int tid, void *msg, int msglen, void *reply, int replylen) {
    if (tid < 0) {
        return ERR_INVALID_TID;
    }

    Message message;
    message.destTid = tid;
    message.msglen = msglen;
    message.replylen = replylen;
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
    if (tid < 0) {
        return ERR_INVALID_TID;
    }

    Message message;
    message.destTid = tid;
    message.msglen = replylen;
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

/* Name Server */

int RegisterAs(char *name) {
    int msglen = 0;
    while (*(name+msglen) != '\0') {
        msglen++;
    }

    NameserverMessage nameserverMessage;
    nameserverMessage.type = NServerMSG_REGAS;
    stringCopy(nameserverMessage.serverName, name, SERVERNAME_MAX_LENGTH);

    int status;
    int result = Send(NAMESERVER_TID, &nameserverMessage, sizeof(NameserverMessage), &status, sizeof(int));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    if (status != NAMESERVER_SUCCESS) {
        return ERR_NOT_NAMESERVER;
    }
    return SUCCESS;
}

int WhoIs(char *name) {
    int msglen = 0;
    while (*(name+msglen) != '\0') {
        msglen++;
    }

    NameserverMessage nameserverMessage;
    nameserverMessage.type = NServerMSG_WHOIS;
    stringCopy(nameserverMessage.serverName, name, SERVERNAME_MAX_LENGTH);

    int tid;
    int result = Send(NAMESERVER_TID, &nameserverMessage, sizeof(NameserverMessage), &tid, sizeof(int));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    if (tid <= 0) {
        return ERR_NOT_NAMESERVER;
    }
    return tid;
}

/* Interrupt Processing */

int AwaitEvent(int eventid) {
    Request request;
    request.syscall = SYS_AWAITEVT;
    request.eventId = eventid;
    return sendRequest(&request);
}

/* Clock Server */

int Delay(int ticks) {
    if (ticks <= 0) {
        /* Delayed time <= 0, it is a pass, ensentially. */
        Pass();
        return SUCCESS;
    }

    /* Get clock server tid. */
    int clockServerTid = WhoIs("Clock Server");
    if (clockServerTid < 0) {
        return clockServerTid;
    }

    /* Send message to clock server. */
    ClockserverMessage message;
    message.type = CServerMSG_CLIENT;
    message.syscall = CServerMSG_DELAY;
    message.data = ticks;

    int data;
    int result = Send(clockServerTid, &message, sizeof(message), &data, sizeof(data));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    if (data <= 0) {
        return ERR_NOT_CLOCKSERVER;
    }
    return SUCCESS;
}

int Time() {
    /* Get Clock Server tid. */
    int clockServerTid = WhoIs("Clock Server");
    if (clockServerTid < 0) {
        return clockServerTid;
    }

    /* Send message to Clock Server requesting time. */
    ClockserverMessage message;
    message.type = CServerMSG_CLIENT;
    message.syscall = CServerMSG_TIME;

    int data;
    int result = Send(clockServerTid, &message, sizeof(message), &data, sizeof(data));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    if (data <= 0) {
        return ERR_NOT_CLOCKSERVER;
    }
    return data;
}

int DelayUntil(int ticks) {
    /* Get Clock Server tid. */
    int clockServerTid = WhoIs("Clock Server");
    if (clockServerTid < 0) {
        return clockServerTid;
    }

    /* Send message to Clock Server. */
    ClockserverMessage message;
    message.type = CServerMSG_CLIENT;
    message.syscall = CServerMSG_UNTIL;
    message.data = ticks;

    int data;
    int result = Send(clockServerTid, &message, sizeof(message), &data, sizeof(data));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    if (data <= 0) {
        return ERR_NOT_CLOCKSERVER;
    }
    return SUCCESS;
}

/* Input/Output */

int Getc(int channel) {
    int ioServerTid;
    switch(channel) {
        case 0:
            ioServerTid = WhoIs("Train IO Server");
            break;
        case 1:
            ioServerTid = WhoIs("Terminal IO Server");
            break;
        default:
            return ERR_INVALID_TID;
    }

     /* Send message to Clock Server. */
    IOserverMessage message;
    message.type = IOServerMSG_CLIENT;
    message.syscall = IOServerMSG_GETC;

    char data;
    int result = Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    if ((int)data <= 0) {
        return ERR_NOT_IOSERVER;
    }
    return data;
}

int Putc(int channel, char ch) {
    int ioServerTid;
    switch(channel) {
        case 0:
            ioServerTid = WhoIs("Train IO Server");
            break;
        case 1:
            ioServerTid = WhoIs("Terminal IO Server");
            break;
        default:
            return ERR_INVALID_TID;
    }

     /* Send message to Clock Server. */
    IOserverMessage message;
    message.type = IOServerMSG_CLIENT;
    message.syscall = IOServerMSG_GETC;
    message.data = ch;

    int data;
    int result = Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    return SUCCESS;
}

/* Internal helper */

int sendRequest(Request* request) {
    /* Request's ptr is already in r0 */
    asm("swi");                         // Call kerent with request
    return request->retVal;
}
