#include <lock.h>
#include <utils.h>
#include <task.h>
#include <scheduler.h>

int lock_acquire(Lock *lock, struct Task *task) {
    taskQueue_push(lock, task);
    assert(task->state == TASK_ACTIVE, "lock_acquire: non-active task try to acquire lock.");
    if (lock->start == task) {
        /* The task has acquired the lock successfully. */
        return 1;
    }
    /* Lock not acquired, task blocked and put into waiting list. */
    task->state = TASK_LOCK_BLK;
    return 0;
}

Task* lock_release(Lock *lock, struct Task *task) {
    if (lock->start != task) {
        warning ("Lock_release: trying to release a lock does not belong to it!");
        return (Task*)0;
    }
    assert(task->state == TASK_ACTIVE, "lock_release: non-active task try to release lock.");

    taskQueue_pop(lock);
    if (taskQueue_empty(lock)) {
        /* Waiting list if empty. */
        return (Task*)0;
    }
    /* Unblock next waiting task. */
    Task *newLockHolder = lock->start;
    assert(newLockHolder->state == TASK_LOCK_BLK, "lock_release: waiting task not blocked on lock");
    newLockHolder->state = TASK_READY;
    return newLockHolder;
}
