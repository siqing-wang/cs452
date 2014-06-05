 /*
  * clockserver.c
  */

#include <clockserver.h>
#include <syscall.h>
#include <event.h>
#include <bwio.h>
#include <utils.h>

void clockNotifier() {
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    ClockserverMessage message;
    message.type = CServerMSG_NOTIFIER;
    for(;;) {
        AwaitEvent(EVENT_TIMER);
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void clockServer() {
    int msg = 0;
    int notifierTid = Create(PRIORITY_HIGH, &clockNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

    int tickCount = 0;

    RegisterAs("Clock Server");

    int requesterTid;
    ClockserverMessage message;
    for(;;) {
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case CServerMSG_NOTIFIER:
                Reply(requesterTid, &msg, sizeof(msg));
                tickCount++;
                bwprintf(COM2, "\033[1;1HCount: %u\n\r", tickCount);
                break;
            case CServerMSG_CLIENT:
                break;
            default :
                warning("Unknown Clockserver Message Type.");
        }
    }
}
