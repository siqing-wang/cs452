/*
 * context_switch.h
 *
 * intent
 *      context switch triggered by hardware interupt.
 *      wrapper around kerent and kerexit. (It would call kerent to enter kernel, when kernel
 *      calls kerexit, it would return to intent before going back to userspace.)
 * kerent
 *		back to kernel, get request from r0 and update request & sp passed into kerxit.
 * kerxit
 *		exit kernel, get return value(r0), spsr(r1), pc(r2), and other saved registers from stack and jump to user space.
 */

#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <task.h>
#include <request.h>

void intent();
void kerent();
void kerxit(int **sp, Request **req);

#endif
