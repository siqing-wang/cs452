 /*
  * user_task.c
  */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <clockserver.h>
#include <bwio.h>
#include <utils.h>

void idleTask() {
    for(;;) {

    }
}

void clockClient() {
    Exit();
}

void firstUserTask() {
    int tid;

    // Create NameServer
    tid = Create(PRIORITY_HIGH, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    Create(PRIORITY_MED, &clockServer);

    int i = 0;
    for(;i<4;i++) {
        Create(PRIORITY_MED - 1, &clockClient);
    }

    Create(PRIORITY_LOW, &idleTask);

    Exit();
}
