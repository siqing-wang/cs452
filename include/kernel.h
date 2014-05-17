/*
 * kernel.h
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "task.h"
#include "request.h" 

void kernel_init();
void kernel_run();
void activate(Task *active, Request *request);

#endif
