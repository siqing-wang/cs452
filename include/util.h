// util.h
// Author: Siqing Wang
// Utility helper function

#ifndef __UTIL_H__
#define __UTIL_H__

#include "bwio.h"

/* System */
void nop();
void memcpy(char *dest, const char *src, int n);
void assert(int i);

/* String */
int stringStartWith( char* s1, char* s2 );
int stringEqual( char* s1, char* s2 );

/* Math */
int getLogValue( int data, int base );
int getExpValue( int data, int exp );

#endif
