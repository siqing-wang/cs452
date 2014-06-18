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
void Putw(int ioServerTid, int n, char fc, char *bf);
void format(int ioServerTid, char *fmt, va_list va);

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

     /* Send message to IO Server. */
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

     /* Send message to IO Server. */
    IOserverMessage message;
    message.type = IOServerMSG_CLIENT;
    message.syscall = IOServerMSG_PUTC;
    message.data = ch;

    int data;
    int result = Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
    if (result < 0) {
        return ERR_INVALID_TID;
    }
    return SUCCESS;
}

int PutStr(int channel, char *str) {
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

     /* Send message to IO Server. */
    IOserverMessage message;
    message.type = IOServerMSG_CLIENT;
    message.syscall = IOServerMSG_PUTC;

    int result, data;
    while(*str) {
        message.data = *str;
        result = Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
        if (result < 0) {
            return result;
        }
        str++;
    }
    return SUCCESS;
}

void Printf(int channel, char *fmt, ...) {
    int ioServerTid;
    switch(channel) {
        case 0:
            ioServerTid = WhoIs("Train IO Server");
            break;
        case 1:
            ioServerTid = WhoIs("Terminal IO Server");
            break;
        default:
            return;
    }

    va_list va;
    va_start(va,fmt);
    format(ioServerTid, fmt, va);
    va_end(va);
}


/* Internal helper */

int sendRequest(Request* request) {
    /* Request's ptr is already in r0 */
    asm("swi");                         // Call kerent with request
    return request->retVal;
}

void Putw(int ioServerTid, int n, char fc, char *bf) {
    char ch;
    char *p = bf;

     /* Send message to IO Server. */
    IOserverMessage message;
    message.type = IOServerMSG_CLIENT;
    message.syscall = IOServerMSG_PUTC;

    int data;
    while(*p++ && n > 0) {
        n--;
    }
    while(n-- > 0) {
        message.data = fc;
        Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
    }
    while((ch = *bf++)) {
        message.data = ch;
        Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
    }
}

void format(int ioServerTid, char *fmt, va_list va) {
    char bf[12];
    char ch, lz;
    int w;

     /* Send message to IO Server. */
    IOserverMessage message;
    message.type = IOServerMSG_CLIENT;
    message.syscall = IOServerMSG_PUTC;

    int data;
    while ((ch = *(fmt++))) {
        if (ch != '%') {
            message.data = ch;
            Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
        }
        else {
            lz = 0; w = 0;
            ch = *(fmt++);
            switch ( ch ) {
                case '0':
                    lz = 1; ch = *(fmt++);
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    ch = a2i( ch, &fmt, 10, &w );
                    break;
            }
            switch( ch ) {
                case 0: return;
                case 'c':
                    message.data = va_arg(va, char);
                    Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
                    break;
                case 's':
                    Putw(ioServerTid, w, 0, va_arg(va, char*));
                    break;
                case 'u':
                    ui2a(va_arg(va, unsigned int), 10, bf);
                    Putw(ioServerTid, w, lz, bf);
                    break;
                case 'd':
                    i2a(va_arg(va, int ), bf);
                    Putw(ioServerTid, w, lz, bf);
                    break;
                case 'x':
                    ui2a(va_arg(va, unsigned int), 16, bf );
                    Putw(ioServerTid, w, lz, bf);
                    break;
                case '%':
                    message.data = ch;
                    Send(ioServerTid, &message, sizeof(message), &data, sizeof(data));
                    break;
            }
        }
    }
}
