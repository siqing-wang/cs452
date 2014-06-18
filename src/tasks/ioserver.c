 /*
  * ioserver.c
  */

#include <ioserver.h>
#include <syscall.h>
#include <event.h>
#include <utils.h>
#include <io.h>
#include <ts7200.h>

void terminalSendNotifier() {
    int serverTid;
    int msg = 0;
    char data;

    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    IOserverMessage message;
    message.type = IOServerMSG_SEND_NOTIFIER;

    for(;;) {
        AwaitEvent(EVENT_TERMINAL_SEND);
        Send(serverTid, &message, sizeof(message), &data, sizeof(data));
        io_putdata(COM2, data);
        io_interrupt_enable(COM2, TIEN_MASK);
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
        message.data = io_getdata(COM2);
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
        AwaitEvent(EVENT_TRAIN_SEND);
        Send(serverTid, &message, sizeof(message), &data, sizeof(data));
        io_putdata(COM1, data);
        io_interrupt_enable(COM1, TIEN_MASK);
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
        AwaitEvent(EVENT_TRAIN_RECV);
        message.data = io_getdata(COM2);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void terminalIOServer() {
    int msg = 0;
    /* Create notifiers, and send a message to it. */
    int notifierTid;
    notifierTid = Create(PRIORITY_HIGH, &terminalSendNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));
    notifierTid = Create(PRIORITY_HIGH, &terminalRecvNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

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
    int notifierTid;
    notifierTid = Create(PRIORITY_HIGH, &terminalSendNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));
    notifierTid = Create(PRIORITY_HIGH, &terminalRecvNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

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
