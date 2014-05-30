 /*
 * kernel.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <scheduler.h>
#include <task_queue.h>
#include <send_queue.h>
#include <user_tasks.h>
#include <test_tasks.h>
#include <syscall.h>
#include <bwio.h>

void hardware_init() {
    /* Enable Cache */
    asm("mrc p15, 0, r0, c1, c0, 0");   // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0198e/I1039296.html
    asm("orr r0, r0, #4096");           // bit 12 for I-cache
    asm("orr r0, r0, #4");              // bit 2 for D-cache
    asm("mcr p15, 0, r0, c1, c0, 0");

    /* Invalid Caches */
    asm("mov r0, #0");
    asm("mcr p15, 0, r0, c7, c7, 0");   // Invalid both I-cache and D-cache
}

void kernel_init(SharedVariables *sharedVariables) {
    /* Setup BWIO */
    bwsetfifo( COM2, OFF );

    /* Store kerent function's address in swi jump table. */
    int * addr = (int *) 0x28;
    *addr = (int)(sharedVariables->loadOffset) + (int)&kerent;

    /* Initialize kernel components. */
    task_init(sharedVariables);
    scheduler_init(sharedVariables);

    /* Create and add first task. */
    // Task *firstTask = task_create(sharedVariables, -1, PRIORITY_MED, &firstTestTask);    // For test only
    Task *firstTask = task_create(sharedVariables, -1, PRIORITY_MED, &firstUserTask);
    scheduler_add(sharedVariables, firstTask);
}

void kernel_run() {
    hardware_init();

    /* Memory allocation and build SharedVairables */

    // Initialization (Scheduler)
    TaskQueue task_queues[PRIORITY_MAX + 1];    // +1 because we want queues for  0~PRIORITY_MAX
    int highestOccupiedQueue = -1;

    // Initialization (Task)
    Task tasks[TASK_MAX_NUM];                   // pre-alloc spaces for all tasks
    SendQueue send_queues[TASK_MAX_NUM];        // pre-alloc spaces for each task's send_queue
    TaskQueue free_list;

    // Initialization (Global)
    register int loadOffset asm ("sl");         // Get stack base from register (normally 0x00218000)

    SharedVariables sharedVariables;
    sharedVariables.task_queues = task_queues;
    sharedVariables.highestOccupiedQueue = &highestOccupiedQueue;
    sharedVariables.tasks = tasks;
    sharedVariables.send_queues = send_queues;
    sharedVariables.free_list = &free_list;
    sharedVariables.loadOffset = loadOffset;


    /* Start Kernel */
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

