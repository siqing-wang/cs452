 /*
  * user_task.c
  */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <clockserver.h>
#include <ioserver.h>
#include <timer.h>
#include <bwio.h>
#include <utils.h>
#include <train.h>

void idleTask() {
    IAmIdleTask();
    for(;;) {

    }
}

void firstUserTask() {

    debugTimer_init();

    /* Create NameServer. */
    int tid = Create(15, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // /* Create Clock Server. */
    tid = Create(13, &clockServer);
    assertEquals(CLOCKSERVER_TID, tid, "ClockServer should be the first task.");

    // /* Create IO Server. */
    tid = Create(11, &trainIOServer);
    assertEquals(TRAINIOSERVER_TID, tid, "TrainIOServer should be the first task.");

    tid = Create(11, &terminalIOServer);
    assertEquals(TERMINALIOSERVER_TID, tid, "TerminalIOServer should be the first task.");

    Create(2, &train);

    /* Create Idle Task. */
    Create(0, &idleTask);


    Exit();
}
