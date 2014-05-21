 /*
 * scheduler.c
 */

#include <scheduler.h>
#include <task_queue.h>

TaskQueue task_queues[PRIORITY_MAX + 1] = {};   // +1 because we want queues for  0~PRIORITY_MAX
int highestOccupied = -1;

void scheduler_init() {
    int i = 0;
    for ( ; i <= PRIORITY_MAX; i++) {
        queue_init(task_queues + i);
    }
    highestOccupied = -1;                   // indicates all priority queues empty
}

void scheduler_add(Task* task) {
    queue_push((task_queues + task->priority), task);
    if (task->priority > highestOccupied) {
        highestOccupied = task->priority;
    }
}

Task* scheduler_getNextTask() {
    if (highestOccupied == -1) {            // all priority queues are empty
        return (Task*) 0;
    }
    Task *task = queue_pop((task_queues + highestOccupied));
    /*
     * Update highestOccupied priority. Loop breaks when found a occupied queue,
     * or highestOccupied == -1 when for loop exits
     */
    for( ; highestOccupied>=0; highestOccupied--) {
        if (!queue_empty((task_queues+highestOccupied))){
            break;
        }
    }
    return task;
}
