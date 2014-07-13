#include <lock.h>
#include <utils.h>
#include <task.h>
#include <scheduler.h>

void lock_init(Lock *lock) {
    lock->holder = (Task*)0;
    taskQueue_init(&(lock->waiting));
}

int lock_acquire(Lock *lock, struct Task *task) {
    assert(task->state == TASK_ACTIVE, "lock_acquire: non-active task try to acquire lock.");
    if (lock->holder == (Task*)0) {
        /* Currently no lock owner, get the lock. */
        lock->holder = task;
        return 1;
    }
    taskQueue_push(&(lock->waiting), task);
    /* Lock not acquired, task blocked and put into waiting list. */
    task->state = TASK_LOCK_BLK;
    return 0;
}

Task* lock_release(Lock *lock, struct Task *task) {
    assert(task->state == TASK_ACTIVE, "lock_release: non-active task try to release lock.");
    if (lock->holder != task) {
        warning ("Lock_release: trying to release a lock does not belong to it!");
        return (Task*)0;
    }

    if (taskQueue_empty(&(lock->waiting))) {
        lock->holder = (Task*)0;
    } else {
        /* Unblock next waiting task. */
        lock->holder = taskQueue_pop(&(lock->waiting));
        assertEquals(TASK_LOCK_BLK, lock->holder->state, "lock_release: waiting task not blocked on lock");
        lock->holder->state = TASK_READY;
    }
    return lock->holder;
}
