#ifndef __LOCK_H__
#define __LOCK_H__

#include <task_queue.h>

struct Task;

typedef struct Lock {
	Task *holder;
	TaskQueue waiting;
} Lock;

void lock_init(Lock *lock);
// Return 1:acquired, 0:not acquired, put into waiting list
int lock_acquire(Lock *lock, struct Task *task);
// Return unblocked new lock holder.
Task* lock_release(Lock *lock, struct Task *task);

#endif
