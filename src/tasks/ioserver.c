 /*
  * ioserver.c
  */

#include <ioserver.h>
#include <syscall.h>
#include <event.h>
#include <utils.h>
#include <io_queue.h>

void terminalSendNotifier() {
    int serverTid;
    int msg = 0;
    char data;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_SEND_NOTIFIER;

    for(;;) {
        Send(serverTid, &message, sizeof(message), &data, sizeof(data));
        AwaitSend(EVENT_TERMINAL_SEND, data);
    }
}

void terminalRecvNotifier() {
    int serverTid;
    int msg = 0;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_RECV_NOTIFIER;

    for(;;) {
        message.data = AwaitRecv(EVENT_TERMINAL_RECV);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void trainSendNotifier() {
    int serverTid;
    int msg = 0;
    char data;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_SEND_NOTIFIER;

    for(;;) {
        Send(serverTid, &message, sizeof(message), &data, sizeof(data));
        AwaitSend(EVENT_TRAIN_SEND, data);
    }
}

void trainRecvNotifier() {
    int serverTid;
    int msg = 0;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_RECV_NOTIFIER;

    for(;;) {
        message.data = AwaitRecv(EVENT_TRAIN_RECV);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void terminalIOServer() {
    int msg = 0;
    /* Create notifiers, and send a message to it. */
    int notifierTid;
    notifierTid = Create(12, &terminalSendNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));
    notifierTid = Create(12, &terminalRecvNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

    IOQueue sendQueue;
    IOQueue recvQueue;
    ioQueue_init(&sendQueue);
    ioQueue_init(&recvQueue);

    /* RegAs only after initialization is done. */
    RegisterAs("Terminal IO Server");

    int requesterTid;
    int sendWaitingTid = -1;
    int recvWaitingTid = -1;
    int idleWaitingTid = -1;
    IOserverMessage message;
    char ch;
    int strPut;
    char *str;
    for(;;) {
        ch = 0;
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case IOServerMSG_SEND_NOTIFIER :
                if (ioQueue_empty(&sendQueue)) {
                    sendWaitingTid = requesterTid;
                }
                else {
                    ch = ioQueue_pop(&sendQueue);
                    Reply(requesterTid, &ch, sizeof(ch));
                }
                break;
            case IOServerMSG_RECV_NOTIFIER:
                ioQueue_push(&recvQueue, message.data);
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case IOServerMSG_CLIENT :
                switch (message.syscall) {
                    case IOServerMSG_PUTC :
                        ioQueue_push(&sendQueue, message.data);
                        Reply(requesterTid, &msg, sizeof(msg));
                        break;
                    case IOServerMSG_PUTSTR :
                        str = message.str;
                        for(strPut = 0; strPut < message.strSize; strPut++) {
                            ioQueue_push(&sendQueue, *(str + strPut));
                        }
                        Reply(requesterTid, &strPut, sizeof(strPut));
                        break;
                    case IOServerMSG_GETC :
                        if (!ioQueue_empty(&recvQueue)) {
                            ch = ioQueue_pop(&recvQueue);
                            Reply(requesterTid, &ch, sizeof(ch));
                        }
                        else {
                            recvWaitingTid = requesterTid;
                        }
                        break;
                    case IOServerMSG_IOIDLE :
                        idleWaitingTid = requesterTid;
                        break;
                    default :
                        warning("Unknown IOserver Syscall.");
                }
                break;
            default :
                warning("Unknown IOserver Message Type.");
        }

        if (!ioQueue_empty(&sendQueue) && (sendWaitingTid >= 0)) {
            ch = ioQueue_pop(&sendQueue);
            Reply(sendWaitingTid, &ch, sizeof(ch));
            sendWaitingTid = -1;
        }
        if (!ioQueue_empty(&recvQueue) && (recvWaitingTid >= 0)) {
            ch = ioQueue_pop(&recvQueue);
            Reply(recvWaitingTid, &ch, sizeof(ch));
            recvWaitingTid = -1;
        }
        if ((sendWaitingTid >= 0) && (idleWaitingTid >= 0)) {
            Reply(idleWaitingTid, &msg, sizeof(msg));
            idleWaitingTid = -1;
        }
    }
}

void trainIOServer() {
    int msg = 0;
    /* Create notifiers, and send a message to it. */
    int notifierTid;
    notifierTid = Create(12, &trainSendNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));
    notifierTid = Create(12, &trainRecvNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

    IOQueue sendQueue;
    IOQueue recvQueue;
    ioQueue_init(&sendQueue);
    ioQueue_init(&recvQueue);

    /* RegAs only after initialization is done. */
    RegisterAs("Train IO Server");

    int requesterTid;
    int sendWaitingTid = -1;
    int recvWaitingTid = -1;
    int idleWaitingTid = -1;
    IOserverMessage message;
    char ch;
    for(;;) {
        ch = 0;
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case IOServerMSG_SEND_NOTIFIER :
                if (ioQueue_empty(&sendQueue)) {
                    sendWaitingTid = requesterTid;
                }
                else {
                    ch = ioQueue_pop(&sendQueue);
                    Reply(requesterTid, &ch, sizeof(ch));
                }
                break;
            case IOServerMSG_RECV_NOTIFIER:
                ioQueue_push(&recvQueue, message.data);
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case IOServerMSG_CLIENT :
                switch (message.syscall) {
                    case IOServerMSG_PUTC :
                        ioQueue_push(&sendQueue, message.data);
                        Reply(requesterTid, &msg, sizeof(msg));
                        break;
                    case IOServerMSG_GETC :
                        if (!ioQueue_empty(&recvQueue)) {
                            ch = ioQueue_pop(&recvQueue);
                            Reply(requesterTid, &ch, sizeof(ch));
                        }
                        else {
                            recvWaitingTid = requesterTid;
                        }
                        break;
                    case IOServerMSG_IOIDLE :
                        idleWaitingTid = requesterTid;
                        break;
                    default :
                        warning("Unknown IOserver Syscall.");
                }
                break;
            default :
                warning("Unknown IOserver Message Type.");
        }

        if (!ioQueue_empty(&sendQueue) && (sendWaitingTid >= 0)) {
            ch = ioQueue_pop(&sendQueue);
            Reply(sendWaitingTid, &ch, sizeof(ch));
            sendWaitingTid = -1;
        }
        if (!ioQueue_empty(&recvQueue) && (recvWaitingTid >= 0)) {
            ch = ioQueue_pop(&recvQueue);
            Reply(recvWaitingTid, &ch, sizeof(ch));
            recvWaitingTid = -1;
        }
        if ((sendWaitingTid >= 0) && (idleWaitingTid >= 0)) {
            Reply(idleWaitingTid, &msg, sizeof(msg));
            idleWaitingTid = -1;
        }
    }
}
