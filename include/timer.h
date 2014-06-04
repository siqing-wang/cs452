// timer.h
// Author: Yu Meng Zhang
// Timer library

#ifndef __TIMER_H__
#define __TIMER_H__

#include <ts7200.h>

#define TIMER3_HZ 508000
#define DEBUG_TIMER_HZ 983000

/* Timer */
void timer_init();
unsigned int timer_getVal();
void timer_clear();
void debugTimer_init();
unsigned int debugTimer_getVal();

#endif
