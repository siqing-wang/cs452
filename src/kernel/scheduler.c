 /*
 * scheduler.c
 */

#include <scheduler.h>
#include <task_queue.h>

void scheduler_init(SharedVariables* sharedVariables) {
    TaskQueue* task_queues = sharedVariables->task_queues;
    int i = 0;
    for ( ; i <= PRIORITY_MAX; i++) {
        taskQueue_init(task_queues + i);
    }
    *(sharedVariables->highestOccupiedQueue) = -1;                      // indicates all priority queues empty
}

void scheduler_add(SharedVariables* sharedVariables, Task* task) {
    TaskQueue* task_queues = sharedVariables->task_queues;
    taskQueue_push((task_queues + task->priority), task);
    if (task->priority > *(sharedVariables->highestOccupiedQueue)) {
        *(sharedVariables->highestOccupiedQueue) = task->priority;
    }
}

Task* scheduler_getNextTask(SharedVariables* sharedVariables) {
    if (*(sharedVariables->highestOccupiedQueue) == -1) {               // all priority queues are empty
        return (Task*) 0;
    }
    TaskQueue* task_queues = sharedVariables->task_queues;
    int highestOccupied = *(sharedVariables->highestOccupiedQueue);     // to avoid repetitive read of memory
    Task *task = taskQueue_pop((task_queues + highestOccupied));
    /*
     * Update highestOccupied priority. Loop breaks when found a occupied queue,
     * or highestOccupied == -1 when for loop exits
     */
    for( ; highestOccupied>=0; highestOccupied--) {
        if (!taskQueue_empty((task_queues+highestOccupied))){
            break;
        }
    }
    *(sharedVariables->highestOccupiedQueue) = highestOccupied;         // update highestOccupiedQueue in shared variables
    return task;
}
