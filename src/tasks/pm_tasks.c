 /*
 * user_task.c
 */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <bwio.h>
#include <timer.h>

#define PM_MSG_SIZE 4

void PeformanceRecvTask() {
    RegisterAs("PM Server");
    int tid;
    char str[PM_MSG_SIZE];
    for(;;) {
        Receive(&tid, str, PM_MSG_SIZE);
        Reply(tid, str, PM_MSG_SIZE);
    }
    Exit();
}

void PeformanceSendTask() {
    debugTimer_init();

    int tid = 2;
    char str[PM_MSG_SIZE];

    unsigned int sum = 0;

    int i = 0;
    for (; i < 100; i++) {
        unsigned int val1 = debugTimer_getVal();
        Send(tid, str, PM_MSG_SIZE, str, PM_MSG_SIZE);
        unsigned int val2 = debugTimer_getVal();
        sum += val2 - val1;
    }

    bwprintf(COM2, "Time Diff : %u(%uus)\n\r", sum/100, sum/100 * 1000 / (DEBUG_TIMER_HZ / 1000));
    Exit();
}


void firstPmTask() {
    // Create NameServer
    Create(PRIORITY_HIGH, &nameServer);

    Create(PRIORITY_MED + 1, &PeformanceRecvTask);
    Create(PRIORITY_MED + 3, &PeformanceSendTask);

    Exit();
}
