/*
 * interrupt.c
 */

#include <interrupt.h>
#include <ts7200.h>
#include <timer.h>
#include <utils.h>

void interrupt_enable();
void interrupt_disable(int interruptId);
void interrupt_clearAll();

void interrupt_init() {
     /* Disable IRQ */
    asm("mrs r0, cpsr");
    asm("orr r0, r0, #128");
    asm("msr cpsr, r0");

    // Kernel initializes ICU


    interrupt_clearAll();
    // For each device
    //  Kernel initializes hardware
    //  Kernel turns on interrupt(s) in the device

    // Initiailize Timer
    timer_clear();
    interrupt_enable(INTERRUPT_TIMER);
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

void interrupt_clearAll() {
    unsigned int *interruptClear;
    interruptClear = (unsigned int *) (VIC1_BASE + INTENCLEAR_OFFSET);
    *interruptClear = 0xffffffff;
    interruptClear = (unsigned int *) (VIC2_BASE + INTENCLEAR_OFFSET);
    *interruptClear = 0xffffffff;
}