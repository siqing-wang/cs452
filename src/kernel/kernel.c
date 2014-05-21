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
    /* Store kerent function's address in swi jump table. */
    int * addr = (int *) 0x28;
    *addr = (int) &kerent;

    /* Initialize kernel components. */
    task_init();
    scheduler_init();

    /* Create and add first task. */
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
            /* No available tasks from scheduler. */
            return;
        }
        Request *request;
        /* Run user task and get user request in userspace. */
        activate(active, &request);
        request_handle(active, request);
    }
}

void activate(Task *active, Request **request) {
    /*
     * Why &(active->sp):
     *      Because sp value may be changed when user task runs
     *      We store a pointer to sp's value.
     *      Later on, in "kerent", it can get this address, and update sp value with this ptr.
     *      As a result, sp value in task structure will be updated
     * Why Reuqest**:
     *      Same reason
     *      Only difference is, sp can be represent by an int and Request is a structure which have to be represented with a ptr
     *      This is why we need ptr to ptr
     */
    kerxit(&(active->sp), request);
}

