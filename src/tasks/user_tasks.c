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

void idleTask() {
    for(;;) {

    }
    Exit();
}

void testTask() {
    PutStr(COM2, "Start\n\r");
    for(;;) {
        char c = Getc(COM2);
        if (c == 'q') {
            break;
        }
        Putc(COM2 , c);
    }
    PutStr(COM2, "End\n\r");
    Exit();
}

void firstUserTask() {
    debugTimer_init();

    /* Create NameServer. */
    int tid = Create(PRIORITY_HIGH, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // /* Create Clock Server. */
    Create(PRIORITY_HIGH - 1, &clockServer);


    // /* Create IO Server. */
    Create(PRIORITY_HIGH - 1, &trainIOServer);
    Create(PRIORITY_HIGH - 1, &terminalIOServer);

    Create(2, &testTask);

    /* Create Idle Task. */
    // Create(0, &idleTask);


    Exit();
}
