/*
 *	utils.c
 */

#include <utils.h>
#include <bwio.h>
#include <ui.h>

void assert(int cond, char* msg) {
    if (!cond) {
        bwputstr(COM2, TCS_RED);
        bwputstr(COM2, "Assertion Failed: ");
        bwputstr(COM2, msg);
        bwputstr(COM2, "\n\r");
        bwputstr(COM2, TCS_RESET);
    }
}

void assertEquals(int expected, int actual, char* msg) {
    if (expected != actual) {
        bwputstr(COM2, TCS_RED);
        bwputstr(COM2, "Assertion Failed: ");
        bwputstr(COM2, msg);
        bwprintf(COM2, " expected = %d, actual = %d\n\r", expected, actual);
        bwputstr(COM2, TCS_RESET);
    }
}

void debug(char *msg) {
    if (DEBUG_MODE) {
        bwputstr(COM2, msg);
        bwputstr(COM2, "\n\r");
    }
}

void warning(char *msg) {
    /* TODO: add red color. */
    bwputstr(COM2, TCS_YELLOW);
    bwputstr(COM2, "WARNING: ");
    bwputstr(COM2, msg);
    bwputstr(COM2, "\n\r");
    bwputstr(COM2, TCS_RESET);
}

void memcopy(char *dest, const char *src, int size) {
    int i;
    unsigned long *destll = (unsigned long*) dest;
    unsigned long *srcll = (unsigned long*) src;
    for (i = 0; i < size / (sizeof(unsigned long)); i++) {
        *destll = *srcll;
        destll++;
        srcll++;
    }
    dest = (char*)destll;
    src = (char*)srcll;
    for (i = 0; i < size % (sizeof(unsigned long)); i++) {
        *dest = *src;
        dest++;
        src++;
    }
}

int computeHash(const char *str) {
    int hash = 0;
    while(*str != '\0') {
        hash = (hash * 29 + (int)(*str)) % HASH_TABLE_SIZE;
        str++;
    }
    return hash;
}

int stringEquals(char* s1, char* s2) {
    for(;;) {
        if ((*s1 == '\0') && (*s2 == '\0')) {
            return 1;
        }
        else if (*s1 == '\0') {
            return 0;
        }
        else if (*s2 == '\0') {
            return 0;
        }
        else if (*s1 != *s2) {
            return 0;
        }
        else {
            s1 ++;
            s2 ++;
        }
    }
}

void stringCopy(char *dest, char* src, int len) {
    int i = 0;
    for (; i < len - 1 ; i++) {
        if (*src == '\0') {
            break;
        }
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

/* XoR shift random. */
unsigned long rand(unsigned long x) {
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    return x * 2685821657736338717LL;
}
