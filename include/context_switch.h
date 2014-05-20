/*
 * context_switch.h
 */

#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

#include <task.h>
#include <request.h>

void kerent();

void kerxit(int **sp, Request **req);

#endif
