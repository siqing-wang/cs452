 /*
 * kernel.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <scheduler.h>
#include <bwio.h>
#include <utils.h>
#include <user_tasks.h>

void kernel_init() {
    int * addr = (int *) 0x28;
    *addr = (int) &kerent;

    task_init();
    scheduler_init();
    Task *firstTask = task_create("first", -1, 7, &firstUserTask);
    scheduler_add(firstTask);
}

void kernel_run() {
    kernel_init();
    int i;
    for( i = 0; i < 10; i++ ) {
        Task *active = scheduler_getNextTask();
        bwprintf(COM2, "loop %d\n\r", i);
        // TODO: check if I can compare like this
        if (active == 0) {
            return;
        }
        Request request;
        activate(active, &request);
        request_handle(&request);
        scheduler_add(active);
    }
}

void activate(Task *active, Request *request) {
    kerxit(active->sp, request);
}

