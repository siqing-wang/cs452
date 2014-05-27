/*
 * request.h - called by kernel, handle syscall requests from user space
 *
 * request_handle
 *		handle syscall request
 * storeRetValue
 *		push syscall return value onto stack
 */

#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <task.h>
#include <shared_variable.h>

#define SYS_CREATE 0
#define SYS_MYTID 1
#define SYS_MYPID 2
#define SYS_PASS 3
#define SYS_EXIT 4
#define SYS_SEND 5
#define SYS_RECV 6
#define SYS_REPLY 7
#define SYS_REGAS 8
#define SYS_WHOIS 9

#define ERR_UNKNOWN_SYSCALL -1
#define ERR_CREATE_TASK_FAIL -2

#define ERR_INVALID_TID -1
#define ERR_NOEXIST_TID -2
#define ERR_INCOMPLETE_SRR_TRANS -3
#define ERR_NOT_REPLY_BLK -3
#define ERR_INSUFFICIENT_SPACE -4
#define ERR_NOT_NAMESERVER -2

#define SUCC_REPLY 0
#define SUCC_REGAS 0

typedef struct Request
{
	int syscall;
	int priority;
	void *code;
} Request;

void request_handle(SharedVariables* sharedVariables, Task* active, Request *request);
void storeRetValue(Task* task, int retVal);

#endif
