 /*
 * kernel.c
 */

#include <kernel.h>


void initialize(Task** ts) {
	initTasks(ts);
	createTask("first");
}

Task* schedule(Task** ts) {
	return *ts;
}

int Create(int priority, void (*code)()) {
    return 0;
}

int MyTid() {
    return 0;
}

int MyParentTid() {
    return 0;
}

void Pass() {
    
}

void Exit() {

}


