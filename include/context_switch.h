/*
 * context_switch.h
 */

#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <bwio.h>
#include <task.h>
#include <request.h>

void kerent();

void kerxit(Task *active, Request *req);

#endif
