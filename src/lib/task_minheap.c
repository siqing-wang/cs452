
#include <task_minheap.h>


void taskMinHeap_init(TaskMinHeap* h) {
    h->size = 0;
}

void taskMinHeap_swap(TaskMinHeap* h, int i1, int i2) {
    if (i1 == i2) {
        return;
    }
    elem* e1 = &h->allElems[i1];
    elem* e2 = &h->allElems[i2];

    int tmpTid = e1->tid;
    int tmpTime = e1->time;
    e1->tid = e2->tid;
    e1->time = e2->time;
    e2->tid = tmpTid;
    e2->time = tmpTime;
}

void taskMinHeap_insert(TaskMinHeap* h, int tid, int time) {

    /* Initialize the new element. */
    h->allElems[h->size].tid = tid;
    h->allElems[h->size].time = time;

    /* Heapify */
    int myIndex = h->size;
    int parentIndex;
    for (;;) {
        parentIndex = (myIndex - 1) / 2;
        if (myIndex == 0 || h->allElems[myIndex].time >= h->allElems[parentIndex].time) {
            break;
        }
        /* Swap me and my parent because I am smaller */
        taskMinHeap_swap(h, myIndex, parentIndex);
        myIndex = parentIndex;
    }

    h->size++;
}

void taskMinHeap_peekMin(TaskMinHeap* h, int* tid, int* time) {
    if (h->size == 0) {
        *tid = -1;
        *time = -1;
        return;
    }
    *tid = h->allElems[0].tid;
    *time = h->allElems[0].time;
}

void taskMinHeap_popMin(TaskMinHeap* h, int* tid, int* time) {

    taskMinHeap_peekMin(h, tid, time);

    /* Swap last element to the root. */
    taskMinHeap_swap(h, h->size - 1, 0);
    h->size--;

    /* Sink root down to right location. */
    int myIndex = 0;

    for(;;) {
        int left = myIndex * 2 + 1;      // left child
        int right = myIndex * 2 + 2;     // right child

        int leftTime = -1;
        int rightTime = -1;
        int myTime = h->allElems[myIndex].time;

        /* Get children's time. */
        if (left < h->size) {
            leftTime = h->allElems[left].time;
        }
        if (right < h->size) {
            rightTime = h->allElems[right].time;
        }

        if (leftTime > 0 &&            // left child exits
            // left child smaller than me.
            leftTime < myTime &&
            // it is smaller than right child or right child does not exist
            (rightTime < 0 || leftTime < rightTime)) {

            taskMinHeap_swap(h, myIndex, left);
            myIndex = left;
        } else if (rightTime > 0 && rightTime < myTime &&
            (leftTime < 0 || rightTime < leftTime)) {

            taskMinHeap_swap(h, myIndex, right);
            myIndex = right;
        } else {
            /* Smaller than both children, stop sinking down. */
            break;
        }
    }
}
