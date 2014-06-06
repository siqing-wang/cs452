 /*
  * user_task.c
  */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <clockserver.h>
#include <timer.h>
#include <bwio.h>
#include <utils.h>

/* Used by name server. */
typedef struct ClockClientMessage
{
    int delay;
    int num;

} ClockClientMessage;

void idleTask() {
    char c;
    for(;;) {
        c = bwgetc(COM2);
        if (c == 'q') {
            break;
        }
    }
    Exit();
}

void clockClient() {
    ClockClientMessage message;
    int msg = 0;
    /* Ask for delay interval and number of times it should delay. */
    Send(MyParentTid(), &msg, sizeof(msg), &message, sizeof(message));

    int myTid = MyTid();
    int i;
    unsigned int val1, val2;
    for(i = 1; i <= message.num; i ++) {
        val1 = debugTimer_getVal() / (DEBUG_TIMER_HZ / 1000);
        Delay(message.delay);
        val2 = debugTimer_getVal() / (DEBUG_TIMER_HZ / 1000);
        bwprintf(COM2, "Task%d: Delay Interval=%d ticks (actual=%ums); %d Delay(s) Completed.\n\r", myTid, message.delay, val2 - val1, i);
    }

    Exit();
}

void firstUserTask() {
    debugTimer_init();

    /* Create NameServer. */
    int tid = Create(PRIORITY_HIGH, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    /* Create Clock Server. */
    Create(PRIORITY_MED, &clockServer);

    /* Create Clock Clients. */
    int tid3 = Create(6, &clockClient);
    int tid4 = Create(5, &clockClient);
    int tid5 = Create(4, &clockClient);
    int tid6 = Create(3, &clockClient);

    Create(0, &idleTask);

    int msg = 0;
    ClockClientMessage message;

    /* Tell each clock client their delay intervel and delayed times. */
    int i;
    for(i = 0; i < 4; i++) {
        Receive(&tid, &msg, sizeof(msg));
        if (tid == tid3) {
            message.delay = 10;
            message.num = 20;
            Reply(tid, &message, sizeof(message));
        } else if (tid == tid4) {
            message.delay = 23;
            message.num = 9;
            Reply(tid, &message, sizeof(message));
        } else if (tid == tid5) {
            message.delay = 33;
            message.num = 6;
            Reply(tid, &message, sizeof(message));
        } else if (tid == tid6) {
            message.delay = 71;
            message.num = 3;
            Reply(tid, &message, sizeof(message));
            break;
        }
    }

    Exit();
}
