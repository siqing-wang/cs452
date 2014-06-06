/*
 * task_minheap.h
 *      Used in clocked server to get the delayed task with min time.
 *      Note: min heap elements ordered by time ticks.
 */

#ifndef __TASK_MINHEAP_H__
#define __TASK_MINHEAP_H__

#include <task.h>

typedef struct elem {
    int tid;
    int time;   // sort by time
} elem;

typedef struct heap {
    elem allElems[TASK_MAX_NUM];
    int size;
} TaskMinHeap;

void taskMinHeap_init(TaskMinHeap* h);
void taskMinHeap_insert(TaskMinHeap* h, int tid, int time);
void taskMinHeap_peekMin(TaskMinHeap* h, int* tid, int* time);
void taskMinHeap_popMin(TaskMinHeap* h, int* tid, int* time);

#endif
