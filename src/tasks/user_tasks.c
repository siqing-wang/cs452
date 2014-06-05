 /*
  * user_task.c
  */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <clockserver.h>
#include <bwio.h>
#include <utils.h>

/* Used by name server. */
typedef struct ClockClientMessage
{
    int delay;
    int num;

} ClockClientMessage;

void idleTask() {
    for(;;) {

    }
}

void clockClient() {
    ClockClientMessage message;
    int msg = 0;
    Send(MyParentTid(), &msg, sizeof(msg), &message, sizeof(message));

    int myTid = MyTid();
    int i;
    for(i = 1; i < message.num; i ++) {
        Delay(message.delay);
        bwprintf(COM2, "Task%d Delay Time : %d Delays Complete : %d\n\r", myTid, message.delay, i);
    }

    Exit();
}

void firstUserTask() {
    int tid;

    // Create NameServer
    tid = Create(PRIORITY_HIGH, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    Create(PRIORITY_MED, &clockServer);

    int tid3 = Create(6, &clockClient);
    int tid4 = Create(5, &clockClient);
    int tid5 = Create(4, &clockClient);
    int tid6 = Create(3, &clockClient);

    Create(0, &idleTask);

    int msg = 0;
    ClockClientMessage message;

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
