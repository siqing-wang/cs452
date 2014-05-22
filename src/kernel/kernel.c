 /*
 * kernel.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <scheduler.h>
#include <task.h>
#include <user_tasks.h>
#include <test_tasks.h>
#include <syscall.h>
#include <bwio.h>

void kernel_init(SharedVariables *sharedVariables) {
    /* Store kerent function's address in swi jump table. */
    int * addr = (int *) 0x28;
    *addr = (int) &kerent;

    /* Initialize kernel components. */
    task_init(sharedVariables);
    scheduler_init(sharedVariables);

    /* Create and add first task. */
    // Task *firstTask = task_create(-1, PRIORITY_MED, &firstTestTask);    // For test only
    Task *firstTask = task_create(sharedVariables, -1, PRIORITY_MED, &firstUserTask);
    scheduler_add(sharedVariables, firstTask);
}

void kernel_run() {
    // Initialization (Queue)
    TaskQueue task_queues[PRIORITY_MAX + 1];    // +1 because we want queues for  0~PRIORITY_MAX
    int highestOccupiedQueue = -1;

    // Initialization (Task)
    Task tasks[TASK_MAX_NUM];                   // pre-alloc spaces for all tasks
    int stack[STACK_SIZE * TASK_MAX_NUM];       // pre-alloc spaces for all tasks' stacks
    int nextTaskId = 0;                         // keep track of next available task slot


    SharedVariables sharedVariables;
    sharedVariables.task_queues = task_queues;
    sharedVariables.highestOccupiedQueue = &highestOccupiedQueue;
    sharedVariables.tasks = tasks;
    sharedVariables.stack = stack;
    sharedVariables.nextTaskId = &nextTaskId;

    kernel_init(&sharedVariables);

    int i;
    for( ;; i++) {
        Task *active = scheduler_getNextTask(&sharedVariables);
        if (active == 0) {
            /* No available tasks from scheduler. */
            return;
        }
        Request *request;
        /* Run user task and get user request in userspace. */
        activate(active, &request);
        request_handle(&sharedVariables, active, request);
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

