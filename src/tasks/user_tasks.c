 /*
 * user_task.c
 */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <message.h>
#include <bwio.h>
#include <utils.h>

void testTask() {
    int i = 0;
    int j = 0;
    for(;i<10000;i++) {
        j = 0;
        for(;j<10000;j++) {

        }
        // bwprintf(COM2, "/");
    }
    bwprintf(COM2, "leave\n\r");
    Exit();
}

void firstUserTask() {
    int tid;

    // Create NameServer
    // tid = Create(PRIORITY_HIGH, &nameServer);
    // assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // Create RPS Server
    tid = Create(PRIORITY_MED + 2, &testTask);

    Exit();
}
