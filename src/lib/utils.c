/*
 *	utils.c
 */

#include <utils.h>
#include <bwio.h>


void assert(int cond, char* msg) {
    if (!cond) {
        bwputstr(COM2, "\033[31mAssertion Failed: ");
        bwputstr(COM2, msg);
        bwputstr(COM2, "\n\r\033[0m");
    }
}

void assertEquals(int expected, int actual, char* msg) {
    if (expected != actual) {
        bwputstr(COM2, "\033[31mAssertion Failed: ");
        bwputstr(COM2, msg);
        bwprintf(COM2, " expected = %d, actual = %d\n\r\033[0m", expected, actual);
    }
}

void debug(char *msg) {
    if (DEBUG_MODE) {
        bwputstr(COM2, msg);
        bwputstr(COM2, "\n\r");
    }
}

void warning(char *msg) {
    bwputstr(COM2, "WARNING: ");
    bwputstr(COM2, msg);
    bwputstr(COM2, "\n\r");
}

void memcopy(char *dest, const char *src, int size) {
    int i = 0;
    for (; i < size ; i++) {
        *dest = *src;
        dest++;
        src++;
    }
}
