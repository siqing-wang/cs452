 /*
 * user_task.c
 */

#include <syscall.h>
#include <bwio.h>
#include <utils.h>

void nameServer() {
    bwprintf(COM2, "---NameServer Initialized---\n\r");
    Exit();
}

void rpsServer() {
    bwprintf(COM2, "---RPS Server Initialized---\n\r");
    Exit();
}

void rpsClient() {
    bwprintf(COM2, "---RPS Client Initialized---\n\r");
    Exit();
}

void firstUserTask() {
    int tid;
    bwprintf(COM2, "---Bootstrap---\n\r");

    // Create NameServer
    tid = Create(PRIORITY_MED - 1, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // Create RPS Server
    tid = Create(PRIORITY_MED - 2, &rpsServer);

    // Create RPS Client
    tid = Create(PRIORITY_MED - 2, &rpsClient);


    bwprintf(COM2, "---Program Finished---\n\r");
    Exit();
}
