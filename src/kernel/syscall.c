 /*
 * syscall.c
 */

#include <syscall.h>

int Create(int priority, void (*code)()) {
    asm("swi");
    return 0;
}

int MyTid() {
	asm("swi");
    return 0;
}

int MyParentTid() {
	asm("swi");
    return 0;
}

void Pass() {
    asm("swi");
}

void Exit() {
	asm("swi");
}
