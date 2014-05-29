/*
 * util.h - all utility helper functions
 *
 * assert
 *      Print msg if condition is not true
 * assertEquals
 *      Print msg if expected and actual is not equal
 * debug
 *      Print msg iff DEBUG_MODE is 1
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#define DEBUG_MODE 1
#define HASH_TABLE_SIZE 13

void assert(int cond, char* msg);
void assertEquals(int expected, int actual, char* msg);
void debug(char *msg);
void warning(char *msg);

void memcopy(char *dest, const char *src, int size);

int computeHash(const char *str);
int stringEquals(char* s1, char* s2);
void stringCopy(char *dest, char* src, int len);

#endif
