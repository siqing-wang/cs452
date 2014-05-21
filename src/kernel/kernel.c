 /*
 * kernel.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <scheduler.h>
#include <user_tasks.h>
#include <test_tasks.h>
#include <syscall.h>
#include <bwio.h>

void kernel_init() {
    int * addr = (int *) 0x28;
    *addr = (int) &kerent;
    task_init();
    scheduler_init();
    // Task *firstTask = task_create(-1, PRIORITY_MED, &firstTestTask);    // For test only
    Task *firstTask = task_create(-1, PRIORITY_MED, &firstUserTask);
    scheduler_add(firstTask);
}

void kernel_run() {
    kernel_init();
    int i;
    for( ;; i++) {
        Task *active = scheduler_getNextTask();
        if (active == 0) {
            return;
        }
        Request *request;
        activate(active, &request);
        request_handle(active, request);
    }
}

void activate(Task *active, Request **request) {
    kerxit(&(active->sp), request);
}

