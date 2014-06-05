 /*
  * clockserver.c
  */

#include <clockserver.h>
#include <syscall.h>
#include <event.h>
#include <task_minheap.h>
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
    TaskMinHeap taskMinHeap;
    taskMinHeap_init(&taskMinHeap);
    RegisterAs("Clock Server");

    int requesterTid;
    ClockserverMessage message;
    for(;;) {
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case CServerMSG_NOTIFIER :
                Reply(requesterTid, &msg, sizeof(msg));
                tickCount++;
                bwprintf(COM2, "\033[1;1HCount: %u\n\r", tickCount);
                break;
            case CServerMSG_CLIENT :
                switch (message.syscall) {
                    case CServerMSG_DELAY :
                        taskMinHeap_insert(&taskMinHeap, requesterTid, message.data + tickCount);
                        break;
                    case CServerMSG_TIME :
                        Reply(requesterTid, &tickCount, sizeof(tickCount));
                        break;
                    case CServerMSG_UNTIL :
                        taskMinHeap_insert(&taskMinHeap, requesterTid, message.data);
                        break;
                    default :
                        warning("Unknown Clockserver Syscall.");
                }
                break;
            default :
                warning("Unknown Clockserver Message Type.");
        }
        int tid;
        int delayUntil;
        for(;;) {
            taskMinHeap_peekMin(&taskMinHeap, &tid, &delayUntil);
            if (delayUntil > tickCount) {
                break;
            }
            taskMinHeap_popMin(&taskMinHeap, &tid, &delayUntil);
            Reply(tid, &tickCount, sizeof(tickCount));
        }
    }
}
