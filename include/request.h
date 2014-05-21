/*
 * request.h
 */

#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <task.h>

#define SYS_CREATE 0
#define SYS_MYTID 1
#define SYS_MYPID 2
#define SYS_PASS 3
#define SYS_EXIT 4

#define ERR_UNKNOWN_SYSCALL -1
#define ERR_CREATE_TASK_FAIL -2

typedef struct Request
{
	int syscall;
	int priority;
	void *code;
} Request;

void request_handle(Task* active, Request *request);
void storeRetValue(Task* task, int retVal);

#endif
