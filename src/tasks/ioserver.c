 /*
  * ioserver.c
  */

#include <ioserver.h>
#include <syscall.h>
#include <event.h>
#include <utils.h>

void terminalSendNotifier() {
    int serverTid;
    int msg = 0;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_SEND_NOTIFIER;
    for(;;) {
        AwaitEvent(EVENT_TERMINAL_SEND);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
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
        AwaitEvent(EVENT_TERMINAL_RECV);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void trainSendNotifier() {
    int serverTid;
    int msg = 0;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_SEND_NOTIFIER;
    for(;;) {
        AwaitEvent(EVENT_TRAIN_SEND);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void trainRecvNotifier() {
    int serverTid;
    int msg = 0;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_SEND_NOTIFIER;
    for(;;) {
        AwaitEvent(EVENT_TRAIN_RECV);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void terminalIOServer() {
    int msg = 0;
    /* Create notifiers, and send a message to it. */
    int sendNotifierTid = Create(PRIORITY_HIGH, &terminalSendNotifier);
    Send(sendNotifierTid, &msg, sizeof(msg), &msg, sizeof(msg));
    int recvNotifierTid = Create(PRIORITY_HIGH, &terminalRecvNotifier);
    Send(recvNotifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

    /* RegAs only after initialization is done. */
    RegisterAs("Terminal IO Server");

    int requesterTid;
    IOserverMessage message;
    for(;;) {
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case IOServerMSG_SEND_NOTIFIER :
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case IOServerMSG_RECV_NOTIFIER:
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case IOServerMSG_CLIENT :
                switch (message.syscall) {
                    case IOServerMSG_PUTC :
                        Reply(requesterTid, &msg, sizeof(msg));
                        break;
                    case IOServerMSG_GETC :
                        break;
                    default :
                        warning("Unknown IOserver Syscall.");
                }
                break;
            default :
                warning("Unknown IOserver Message Type.");
        }
    }
}

void trainIOServer() {
    int msg = 0;
    /* Create notifiers, and send a message to it. */
    int sendNotifierTid = Create(PRIORITY_HIGH, &trainSendNotifier);
    Send(sendNotifierTid, &msg, sizeof(msg), &msg, sizeof(msg));
    int recvNotifierTid = Create(PRIORITY_HIGH, &trainRecvNotifier);
    Send(recvNotifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

    /* RegAs only after initialization is done. */
    RegisterAs("Train IO Server");

    int requesterTid;
    IOserverMessage message;
    for(;;) {
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case IOServerMSG_SEND_NOTIFIER :
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case IOServerMSG_RECV_NOTIFIER:
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case IOServerMSG_CLIENT :
                switch (message.syscall) {
                    case IOServerMSG_PUTC :
                        Reply(requesterTid, &msg, sizeof(msg));
                        break;
                    case IOServerMSG_GETC :
                        break;
                    default :
                        warning("Unknown IOserver Syscall.");
                }
                break;
            default :
                warning("Unknown IOserver Message Type.");
        }
    }
}
