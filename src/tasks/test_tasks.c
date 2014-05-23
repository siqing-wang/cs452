 /*
 * test_tasks.c
 */

#include <syscall.h>
#include <bwio.h>

void tinyTask() {
    bwprintf(COM2, "Tiny Task\n");
    Exit();
}

void verySmallTask() {
    Pass();
    bwprintf(COM2, "Very Small Task\n");
    Exit();
}

void smallTask() {
    bwprintf(COM2, "Tid: %d, Parent_Tid: %d.\n", MyTid(), MyParentTid());
    Exit();
}

// All tasks' priority are the same higher than current
void testTask1() {
    int i;
    int tid;
    for (i=0; i< 10; i++) {
        tid = Create(10, &tinyTask);
        bwprintf(COM2, "Task1 Created: %d.\n", tid);
    }
    for (i=0; i< 10; i++) {
        Pass();
    }
    bwprintf(COM2, "----------\n");
    for (i=0; i< 10; i++) {
        tid = Create(10, &smallTask);
        bwprintf(COM2, "Task1 Created: %d.\n", tid);
    }
    Exit();
}

// All tasks' priority are the same lower than current
void testTask2() {
    int i;
    int tid;
    for (i=0; i< 10; i++) {
        tid = Create(9, &tinyTask);
        bwprintf(COM2, "Task2 Created: %d.\n", tid);
    }
    for (i=0; i< 10; i++) {
        Pass();
    }
    bwprintf(COM2, "----------\n");
    for (i=0; i< 10; i++) {
        tid = Create(9, &smallTask);
        bwprintf(COM2, "Task2 Created: %d.\n", tid);
    }
    Exit();
}

// All tasks' priority are the same as current
void testTask3() {
    int i;
    int tid;
    for (i=0; i< 10; i++) {
        tid = Create(9, &tinyTask);
        bwprintf(COM2, "Task3 Created: %d.\n", tid);
    }
    for (i=0; i< 10; i++) {
        Pass();
    }
    bwprintf(COM2, "----------\n");
    for (i=0; i< 10; i++) {
        tid = Create(9, &verySmallTask);
        bwprintf(COM2, "Task3 Created: %d.\n", tid);
    }
    for (i=0; i< 10; i++) {
        Pass();
    }
    bwprintf(COM2, "----------\n");
    for (i=0; i< 10; i++) {
        tid = Create(9, &smallTask);
        bwprintf(COM2, "Task3 Created: %d.\n", tid);
    }
    Exit();
}

// A lot of tiny tasks
void testTask4() {
    int i;
    int tid;
    for (i=0; i< 60; i++) {
        tid = Create(10, &tinyTask);
        bwprintf(COM2, "Task4 Created: %d.\n", tid);
    }
    Exit();
}

// Min/Max/Invalid priority
void testTask5() {
    int tid;
    tid = Create(0, &tinyTask);
    bwprintf(COM2, "Task5 Created: %d.\n", tid);
    tid = Create(-1, &tinyTask);
    bwprintf(COM2, "Task5 Created: %d.\n", tid);
    tid = Create(15, &tinyTask);
    bwprintf(COM2, "Task5 Created: %d.\n", tid);
    tid = Create(16, &tinyTask);
    bwprintf(COM2, "Task5 Created: %d.\n", tid);
    Exit();
}

// A lot of tasks
void testTask6() {
    int i;
    int tid;
    for (i=0; i< 60; i++) {
        tid = Create(8, &verySmallTask);
        bwprintf(COM2, "Task6 Created: %d.\n", tid);
    }
    Exit();
}

void firstTestTask() {
    // Main test task
    int tid;
    bwprintf(COM2,"Tests Start...\n\r");

    // Test 0
    bwprintf(COM2, "-----Test0 Start-----\n", tid);
    bwprintf(COM2, "First Task Tid: %d, Parent_Tid: %d.\n", MyTid(), MyParentTid());
    bwprintf(COM2, "-----Test0 End-----\n", tid);

    // Test 1
    bwprintf(COM2, "-----Test1 Start-----\n", tid);
    tid = Create(8, &testTask1);
    bwprintf(COM2, "-----Test1 End-----\n", tid);

    // Test 2
    bwprintf(COM2, "-----Test2 Start-----\n", tid);
    tid = Create(10, &testTask2);
    bwprintf(COM2, "-----Test2 End-----\n", tid);

    // Test 3
    bwprintf(COM2, "-----Test3 Start-----\n", tid);
    tid = Create(9, &testTask3);
    bwprintf(COM2, "-----Test3 End-----\n", tid);

    // Test 4
    bwprintf(COM2, "-----Test4 Start-----\n", tid);
    tid = Create(9, &testTask4);
    bwprintf(COM2, "-----Test4 End-----\n", tid);

    // Test 5
    bwprintf(COM2, "-----Test5 Start-----\n", tid);
    tid = Create(9, &testTask5);
    bwprintf(COM2, "-----Test5 End-----\n", tid);

    // Test 6
    bwprintf(COM2, "-----Test6 Start-----\n", tid);
    tid = Create(9, &testTask6);
    bwprintf(COM2, "-----Test6 End-----\n", tid);

    // End
    bwprintf(COM2,"All Tests Finished...\n\r");
    Exit();
}
