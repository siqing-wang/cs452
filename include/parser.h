// parser.h
// Author: Yu Meng Zhang
// Parser and Parser helper function

#ifndef __PARSER_H__
#define __PARSER_H__

#include <util.h>

#define CMD_FAILED  	0
#define CMD_SUCCEED  	1
#define CMD_HALT  		2
#define CMD_PM_ON 		3
#define CMD_PM_OFF 		4

/* Parser Helper */
int a2d( char ch );
char a2i( char ch, char **src, int base, int *nump );
void ui2a( unsigned int num, unsigned int base, char *bf );
void i2a( int num, char *bf );
int readNum(char** input);
int isWhiteSpace(char c);

/* Parser */
int skipWhiteSpace(char** input);
void readToken( char** orig, char* result);
int readStrictToken(char** input, char* token);

#endif
