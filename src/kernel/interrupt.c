/*
 * interrupt.c
 */

#include <interrupt.h>
#include <event.h>
#include <scheduler.h>
#include <ts7200.h>
#include <timer.h>
#include <io.h>
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

    /* Initiailize Timer */
    timer_clear();
    timer_init();
    io_init(COM1);
    io_init(COM2);
    interrupt_enable(INTERRUPT_TIMER);
    interrupt_enable(INTERRUPT_TERMINAL);
    interrupt_enable(INTERRUPT_TRAIN);
}

void interrupt_handle(SharedVariables* sharedVariables, Task* active) {
    char ch = 0;

    if (interrupt_check(INTERRUPT_TIMER)) {
        timer_clear();
        event_unblockTask(sharedVariables, EVENT_TIMER, ch);
    }
    else if (interrupt_check(INTERRUPT_TERMINAL)) {
        int *interruptVal = (int *) (UART2_BASE + UART_INTR_OFFSET);

        switch((*interruptVal) & INTR_MASK) {
            case RIS_MASK:
                ch = io_getdata(COM2);
                event_unblockTask(sharedVariables, EVENT_TERMINAL_RECV, ch);
                break;
            case TIS_MASK:
                io_interrupt_disable(COM2, TIEN_MASK);
                sharedVariables->com2TxReady = 1;
                event_unblockTask(sharedVariables, EVENT_TERMINAL_SEND, ch);
                break;
            default:
                break;
        }
    }
    else if (interrupt_check(INTERRUPT_TRAIN)) {
        int *interruptVal = (int *) (UART1_BASE + UART_INTR_OFFSET);
        int *flag = (int *) (UART1_BASE + UART_FLAG_OFFSET);
        int ctsStatus = (*flag & CTS_MASK);

        int mask = ((*interruptVal) & INTR_MASK);
        if (mask & MIS_MASK) {
            sharedVariables->com1CtsReady = ctsStatus;
            if (ctsStatus && sharedVariables->com1TxReady) {
                event_unblockTask(sharedVariables, EVENT_TRAIN_SEND, ch);
            }
            *interruptVal = (int)(*interruptVal) & (~MIS_MASK);
        }
        if (mask & RIS_MASK) {
            ch = io_getdata(COM1);
            event_unblockTask(sharedVariables, EVENT_TRAIN_RECV, ch);
        }
        if (mask & TIS_MASK) {
            io_interrupt_disable(COM1, TIEN_MASK);
            sharedVariables->com1TxReady = 1;
            sharedVariables->com1CtsReady = ctsStatus;
            if (sharedVariables->com1CtsReady) {
                event_unblockTask(sharedVariables, EVENT_TRAIN_SEND, ch);
            }
        }
    }
    /* Add current interupted task back to scheduler. */
    if (active->state == TASK_ACTIVE) {
        active->state = TASK_READY;
        scheduler_add(sharedVariables, active);
    }
}

void interrupt_reset() {
    interrupt_clearAll();

    /* Disable Timer */
    timer_clear();
    interrupt_disable(INTERRUPT_TIMER);

    /* Disable IO*/
    interrupt_disable(INTERRUPT_TERMINAL);
    interrupt_disable(INTERRUPT_TRAIN);
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
    unsigned int *interruptStatus;
    /* Check if interupt has occured. */
    if (interruptId < 32) {
        interruptStatus = (unsigned int *) (VIC1_BASE + IRQSTATUS_OFFSET);
        return (unsigned int)(*interruptStatus) & (1 << interruptId);
    } else {
        interruptStatus = (unsigned int *) (VIC2_BASE + IRQSTATUS_OFFSET);
        return (unsigned int)(*interruptStatus) & (1 << (interruptId-32));
    }
}

void interrupt_clearAll() {
    /* Reset all interrupt bit. */
    unsigned int *interruptClear;
    interruptClear = (unsigned int *) (VIC1_BASE + INTENCLEAR_OFFSET);
    *interruptClear = 0xffffffff;
    interruptClear = (unsigned int *) (VIC2_BASE + INTENCLEAR_OFFSET);
    *interruptClear = 0xffffffff;
}
