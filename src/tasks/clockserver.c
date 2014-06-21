 /*
  * clockserver.c
  */

#include <clockserver.h>
#include <syscall.h>
#include <event.h>
#include <task_minheap.h>
#include <utils.h>

void clockNotifier() {
    int serverTid;
    int msg = 0;

    /* Why use send/reply to know who is clock server instead of WhoIs:
     *  Because:
     *      1. Clockserver should not call RegAs before initialization is completed.
     *      2. Clockserver initialization completion depends on notifier ready.
     *      3. Notifier needs to know Clock Server id to be ready.
     *  Chick-egg problem! >.<
     */
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
    /* Create notifier, and send a message to it. */
    int notifierTid = Create(14, &clockNotifier);
    Send(notifierTid, &msg, sizeof(msg), &msg, sizeof(msg));

    int tickCount = 0;
    TaskMinHeap taskMinHeap;
    taskMinHeap_init(&taskMinHeap);
    /* RegAs only after initialization is done. */
    RegisterAs("Clock Server");

    int requesterTid;
    ClockserverMessage message;
    for(;;) {
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.type) {
            case CServerMSG_NOTIFIER :
                /* Msg from notifier: clock has ticked. */
                /* Important Note: Need to reply to unblock notififer ASAP to avoid missing ticks. */
                Reply(requesterTid, &msg, sizeof(msg));
                tickCount++;
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
            /* Unblock all tasks with delayed time reached. */
            taskMinHeap_peekMin(&taskMinHeap, &tid, &delayUntil);
            if ((delayUntil == -1) || (delayUntil > tickCount)) {
                break;
            }
            taskMinHeap_popMin(&taskMinHeap, &tid, &delayUntil);
            Reply(tid, &tickCount, sizeof(tickCount));
        }
    }
}
