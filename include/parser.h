// parser.h
// Author: Yu Meng Zhang
// Parser and Parser helper function

#ifndef __PARSER_H__
#define __PARSER_H__

#define CMD_FAILED      0
#define CMD_SUCCEED     1
#define CMD_HALT        2
#define CMD_PM_ON       3
#define CMD_PM_OFF      4

/* Parser Helper */
int readNum(char** input);
int isWhiteSpace(char c);

/* Parser */
int skipWhiteSpace(char** input);
int readToken(char** input, char* token);

/* Parse Commands. */
int parseCommand(char* input);
int parseSetSpeedCommand(char* input);
int parseReverseDirectionCommand(char* input);
int parseTurnSwitchCommand(char* input) ;
int parseHaltCommand(char* input);
int parsePerformanceMonitor(char* input);
#endif
