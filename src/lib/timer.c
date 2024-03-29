/* timer.c - timer library */

#include <timer.h>

/* Timer */

void timer_init() {
    int *timerLoad = (int *)(TIMER3_BASE + LDR_OFFSET);
    *timerLoad = TIMER3_HZ / 100; // precious to 10 millisec
    int *timerCtr = (int *)(TIMER3_BASE + CRTL_OFFSET);
    *timerCtr = ENABLE_MASK | CLKSEL_MASK | MODE_MASK;
}

unsigned int timer_getVal() {
    int *timerVal = (int *) (TIMER3_BASE + VAL_OFFSET);
    return *timerVal;
}

void timer_clear() {
    int* timerClear = (int*)(TIMER3_BASE + CLR_OFFSET);
    *timerClear = 1;
}

void debugTimer_init() {
    int *timerCtr = (int *)(TIMER4_BASE + TIMER4_CTRL_OFFSET);
    *timerCtr = TIMER4_ENABLE_MASK | *timerCtr;
}

unsigned int debugTimer_getVal() {
    int *timerVal = (int *) (TIMER4_BASE);
    return *timerVal;
}
