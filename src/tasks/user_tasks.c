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
}

void testTask() {
    PutStr(COM2, "Start\n\r");
    int i=0;
    for(;;i++) {
        char c = Getc(COM2);
        if (c > 0)
            Putc(COM2 , c);
        if (c == 'q') {
            break;
        }
    }
    PutStr(COM2, "End\n\r");
    Exit();
}

void firstUserTask() {
    debugTimer_init();

    /* Create NameServer. */
    int tid = Create(15, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // /* Create Clock Server. */
    Create(13, &clockServer);


    // /* Create IO Server. */
    Create(11, &trainIOServer);
    Create(11, &terminalIOServer);

    Create(2, &testTask);

    /* Create Idle Task. */
    Create(0, &idleTask);


    Exit();
}
