 /*
 * scheduler.c
 */

#include <scheduler.h>
#include <task_queue.h>
#include <bwio.h>
#include <utils.h>

static TaskQueue task_queues[PRIORITY_MAX + 1];
static int highestOccupied;

void scheduler_init() {
	int i = 0;

	for ( ; i <= PRIORITY_MAX; i++) {
		queue_init(task_queues + i);
	}
	highestOccupied = -1;
}

void scheduler_add(Task* task) {
	queue_push((task_queues + task->priority), task);
	if (task->priority > highestOccupied) {
		highestOccupied = task->priority;
	}
}

Task* scheduler_getNextTask() {
	if (highestOccupied == -1) {
		return (Task*) 0;
	}
	Task *task = queue_pop((task_queues + highestOccupied));
	for( ; highestOccupied>=0; highestOccupied--) {
		if (!queue_empty((task_queues+highestOccupied))){
			break;
		}
	}
	return task;
}
