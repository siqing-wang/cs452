 /*
 * user_task.c
 */

#include <syscall.h>
#include <bwio.h>

void otherUserTask() {
    bwprintf(COM2, "Tid: %d, Parent_Tid: %d.\n\r", MyTid(), MyParentTid());
    Pass();
    bwprintf(COM2, "Tid: %d, Parent_Tid: %d.\n\r", MyTid(), MyParentTid());
    Exit();
}

void firstUserTask() {
    int tid;
    tid = Create(1, &otherUserTask);
    bwprintf(COM2, "Created: %d.\n\r", tid);
    tid = Create(1, &otherUserTask);
    bwprintf(COM2, "Created: %d.\n\r", tid);
    tid = Create(15, &otherUserTask);
    bwprintf(COM2, "Created: %d.\n\r", tid);
    tid = Create(15, &otherUserTask);
    bwprintf(COM2, "Created: %d.\n\r", tid);
    bwprintf(COM2, "First: exiting\n\r");
    Exit();
}
