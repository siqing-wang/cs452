 /*
 * scheduler.c
 */

#include <scheduler.h>
#include <task_queue.h>

static TaskQueue* task_queues = 0;
static int* highestOccupied = 0;

void scheduler_init(TaskQueue* t_queues, int* highestOccupiedQueue) {
	task_queues = t_queues;
	highestOccupied = highestOccupiedQueue;
    int i = 0;
    for ( ; i <= PRIORITY_MAX; i++) {
        queue_init(task_queues + i);
    }
    *highestOccupied = -1;               	// indicates all priority queues empty
}

void scheduler_add(Task* task) {
    queue_push((task_queues + task->priority), task);
    if (task->priority > *highestOccupied) {
        *highestOccupied = task->priority;
    }
}

Task* scheduler_getNextTask() {
    if (*highestOccupied == -1) {         	// all priority queues are empty
        return (Task*) 0;
    }
    int highestOccupiedQueue = *highestOccupied;
    Task *task = queue_pop((task_queues + highestOccupiedQueue));
    /*
     * Update highestOccupied priority. Loop breaks when found a occupied queue,
     * or highestOccupied == -1 when for loop exits
     */
    for( ; highestOccupiedQueue>=0; highestOccupiedQueue--) {
        if (!queue_empty((task_queues+highestOccupiedQueue))){
            break;
        }
    }
    *highestOccupied = highestOccupiedQueue;
    return task;
}
