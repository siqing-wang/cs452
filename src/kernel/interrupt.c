/*
 * interrupt.c
 */

#include <interrupt.h>
#include <event.h>
#include <scheduler.h>
#include <ts7200.h>
#include <timer.h>
#include <utils.h>

void interrupt_enable();
void interrupt_disable(int interruptId);
int interrupt_check(int interruptId);
void interrupt_clearAll();

void interrupt_init(SharedVariables* sharedVariables) {
    interrupt_clearAll();
    // For each device
    //  Kernel initializes hardware
    //  Kernel turns on interrupt(s) in the device

    // Initiailize Timer
    timer_clear();
    timer_init();
    event_addInterrupt(sharedVariables, EVENT_TIMER, INTERRUPT_TIMER);
    interrupt_enable(INTERRUPT_TIMER);
}

void interrupt_handle(SharedVariables* sharedVariables, Task* active) {
    if (interrupt_check(INTERRUPT_TIMER)) {
        debug("Timer interrupt");
        timer_clear();

        /* Add current task back to scheduler. */
        if (active->state == TASK_ACTIVE) {
            active->state = TASK_READY;
            scheduler_add(sharedVariables, active);
        }
    }
}

void interrupt_reset() {
    interrupt_clearAll();

    // Disable Timer
    timer_clear();
    interrupt_disable(INTERRUPT_TIMER);
}

void interrupt_enable(int interruptId) {
    assert((interruptId >= 0) && (interruptId < 64), "Invalid Interrupt Id");
    unsigned int *interruptEnable;
    if (interruptId < 32) {
        interruptEnable = (unsigned int *) (VIC1_BASE + INTENABLE_OFFSET);
        *interruptEnable = (unsigned int)(*interruptEnable) | (1 << interruptId);
    } else {
        interruptEnable = (unsigned int *) (VIC2_BASE + INTENABLE_OFFSET);
        *interruptEnable = (unsigned int)(*interruptEnable) | (1 << (interruptId-32));
    }
}

void interrupt_disable(int interruptId) {
    assert((interruptId >= 0) && (interruptId < 64), "Invalid Interrupt Id");
    unsigned int *interruptEnable;
    if (interruptId < 32) {
        interruptEnable = (unsigned int *) (VIC1_BASE + INTENCLEAR_OFFSET);
        *interruptEnable = (unsigned int)(*interruptEnable) | (1 << interruptId);
    } else {
        interruptEnable = (unsigned int *) (VIC2_BASE + INTENCLEAR_OFFSET);
        *interruptEnable = (unsigned int)(*interruptEnable) | (1 << (interruptId-32));
    }
}

int interrupt_check(int interruptId) {
    assert((interruptId >= 0) && (interruptId < 64), "Invalid Interrupt Id");
    unsigned int *interruptEnable;
    if (interruptId < 32) {
        interruptEnable = (unsigned int *) (VIC1_BASE + IRQSTATUS_OFFSET);
        return (unsigned int)(*interruptEnable) & (1 << interruptId);
    } else {
        interruptEnable = (unsigned int *) (VIC2_BASE + IRQSTATUS_OFFSET);
        return (unsigned int)(*interruptEnable) & (1 << (interruptId-32));
    }
}

void interrupt_clearAll() {
    unsigned int *interruptClear;
    interruptClear = (unsigned int *) (VIC1_BASE + INTENCLEAR_OFFSET);
    *interruptClear = 0xffffffff;
    interruptClear = (unsigned int *) (VIC2_BASE + INTENCLEAR_OFFSET);
    *interruptClear = 0xffffffff;
}
