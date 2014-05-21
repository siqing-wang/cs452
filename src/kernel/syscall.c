 /*
 * syscall.c
 */

#include <syscall.h>
#include <request.h>
#include <asm_lib.h>

int Create(int priority, void (*code)()) {
    Request request;
    request.syscall = SYS_CREATE;
    request.priority = priority;
    request.code = code;
    storeRequestInR0(&request);
    asm("swi");
    register int retVal asm ("r0");
    return retVal;
}

int MyTid() {
    Request request;
    request.syscall = SYS_MYTID;
    storeRequestInR0(&request);
    asm("swi");
    register int retVal asm ("r0");
    return retVal;
}

int MyParentTid() {
    Request request;
    request.syscall = SYS_MYPID;
    storeRequestInR0(&request);
    asm("swi");
    register int retVal asm ("r0");
    return retVal;
}

void Pass() {
    Request request;
    request.syscall = SYS_PASS;
    storeRequestInR0(&request);
    asm("swi");
}

void Exit() {
    Request request;
    request.syscall = SYS_EXIT;
    storeRequestInR0(&request);
    asm("swi");
}

