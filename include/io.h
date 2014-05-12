// io.h
// Author: Yu Meng Zhang
// Non-Busy Waiting input and output library

#ifndef __IO_H__
#define __IO_H__
 
#include <ts7200.h>
#include <bwio.h>
#include <parser.h>

#define OB1_SPACE 	512
#define OB2_SPACE 	512

/* IO */
void ioinit(char* b, char* b2);

int getc(int channel);				// return c - succeed, -1 - o.w.
void putc(int channel, char c);		// return 1 - putc succeed, 0 - o.w.
void putstr(int channel, char *str);
void printf(int channel, char *format, ... );
int ob_print(int channel);

#endif
