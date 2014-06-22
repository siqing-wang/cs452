/*
 * request.h
 *      called by kernel, handle syscall requests from user space
 *
 *  request_handle
 *		handle syscall request
 */

#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <task.h>
#include <shared_variable.h>
#include <message.h>

#define SYS_CREATE 0
#define SYS_MYTID 1
#define SYS_MYPID 2
#define SYS_PASS 3
#define SYS_EXIT 4
#define SYS_SEND 5
#define SYS_RECV 6
#define SYS_REPLY 7
#define SYS_AWAITEVT 8
#define SYS_EXIT_PROGRAM 9
#define SYS_IAM_IDLE_TASK 10
#define SYS_IDLE_PERCENT 11
#define SYS_TURN_MONITOR 12

#define ERR_UNKNOWN_SYSCALL -1
#define ERR_CREATE_TASK_FAIL -2

#define ERR_INVALID_REPLY -4

#define ERR_INVALID_TID -1
#define ERR_NOEXIST_TID -2
#define ERR_INCOMPLETE_SRR_TRANS -3
#define ERR_NOT_REPLY_BLK -3
#define ERR_INSUFFICIENT_SPACE -4
#define ERR_NOT_NAMESERVER -2
#define ERR_NOT_CLOCKSERVER -2
#define ERR_NOT_IOSERVER -2

#define NO_RECEIVED_MSG 0
#define HAS_RECEIVED_MSG 1

#define SUCCESS 0

typedef struct Request
{
    int syscall;
    int retVal;             // Return value of the request handle by kernel.

    /* Task Creation */
    int priority;
    void *code;

    /* Inter-task Communication */
    Message *message;

    /* Interrupt Processing */
    int eventId;
    char data;
} Request;

int request_handle(SharedVariables* sharedVariables, Task* active, Request *request);

#endif
