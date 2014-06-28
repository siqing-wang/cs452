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

typedef char *va_list;

#define __va_argsiz(t)  (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)	((void)0)

#define va_arg(ap, t)   (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#define COM1    0
#define COM2    1

#define ON      1
#define OFF     0

/* Printing */
void assert(int cond, char* msg);
void assertEquals(int expected, int actual, char* msg);
void debug(char *msg);
void warning(char *msg);
void displayTime(unsigned int timerCount, int row, int col);

/* Memory */
void memcopy(char *dest, const char *src, int size);

/* String */
int stringStartWith(char* s1, char* s2);
int stringLen(const char *s);                     // NOTE: length does not include \0
int stringEquals(char* s1, char* s2);
void stringCopy(char *dest, char* src, int len);

/* Numbers */
int a2d(char ch);
char a2i(char ch, char **src, int base, int *nump);
void ui2a(unsigned int num, unsigned int base, char *bf);
void i2a(int num, char *bf);

/* Misc */
int putwToBuffer(char *buf, int n, char fc, char *bf);
int computeHash(const char *str);
unsigned long rand(unsigned long x);


#endif
