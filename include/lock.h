#ifndef __LOCK_H__
#define __LOCK_H__

#include <task_queue.h>


typedef TaskQueue Lock;
struct Task;

void lock_init(Lock *lock);
// Return 1:acquired, 0:not acquired, put into waiting list
int lock_acquire(Lock *lock, struct Task *task);
// Return unblocked new lock holder.
Task* lock_release(Lock *lock, struct Task *task);

#endif
