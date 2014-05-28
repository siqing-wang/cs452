/*
 * kernel.h - microkernel
 *
 * kernel_init
 *		Initialize components in kernel (task, scheduler),
 *		and create the first task
 * kernel_run
 *		Call kernel_init and start the kernel
 * activate
 *		Call kerxit and wait for the syscall request from the userspace
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <task.h>
#include <request.h>
#include <shared_variable.h>

void kernel_init(SharedVariables* sharedVariables);
void kernel_run();
void activate(Task *active, Request **request);

#endif
