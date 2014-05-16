 /*
 * kernel.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <scheduler.h>
#include <task.h>
#include <request.h>

void kernel_init() {
    int * y = (int *) 0x28;
    *y = (int) &kerent;

    task_init();
    // createTask("first");
    scheduler_init();
}

void kernel_run() {
    kernel_init();
    int i;
    for( i = 0; i < 4; i++ ) {
        // Task* active = schedule(ts);
        // kerxit(active, req); // req is a pointer to a Request
        // handle( tds, req );
    }
}
