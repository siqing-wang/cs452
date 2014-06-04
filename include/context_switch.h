/*
 * context_switch.h
 *
 * kerent
 *		back to kernel, get request from r0 and update request & sp passed into kerxit
 * kerxit
 *		exit kernel, get return value(r0), spsr(r1), pc(r2), and other saved registers from stack and jump to user space
 */

#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <task.h>
#include <request.h>

void intent();
void kerent();
void kerxit(int **sp, Request **req);

#endif
