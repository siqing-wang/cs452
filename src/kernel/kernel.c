 /*
 * kernel.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <scheduler.h>
#include <task.h>
#include <request.h>
#include <bwio.h>
#include <utils.h>

void kernel_init() {
    int * addr = (int *) 0x28;
    *addr = (int) &kerent;

    task_init();
    scheduler_init();
    Task *firstTask = task_create("first", -1, PRIORITY_MAX);
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
            debug("active == 0 return;");
            return;
        }
        scheduler_add(active);
        // kerxit(active, req); // req is a pointer to a Request
        // handle( tds, req );
    }
}
