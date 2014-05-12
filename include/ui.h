// ui.h
// Author: Yu Meng Zhang
// API for user interface

#ifndef __UI_H__
#define __UI_H__

#include <ts7200.h>
#include <io.h>

/* Terminal control sequence */
#define TCS_RESET	"\033[0m"
#define TCS_RED		"\033[31m"
#define TCS_BLACK	"\033[30m"
#define TCS_GREEN	"\033[32m"
#define TCS_YELLOW	"\033[33m"
#define TCS_BLUE	"\033[34m"
#define TCS_MAGENTA	"\033[35m"
#define TCS_CYAN	"\033[36m"
#define TCS_WHITE	"\033[37m"

/* Display Positions */
#define PM_R 	1 			// timer
#define PM_C 	1

#define TIMER_R 	4 			// timer
#define TIMER_C 	60

#define SWTABLE_R 	8			// switch table
#define SWTABLE_C 	5
#define SWTABLE_NPERLINE 5

#define SENTABLE_R 	16			// sensor table
#define SENTABLE_C 	11
#define SENTABLE_SIZE 6

#define LOG_R 		19			// system log
#define LOG_C 		5

#define CMD_R 		24			// command
#define CMD_C 		7

#define END_R  		26

/* Timer */
void displayTime(int channel, int timerCount);

/* Command line Operations */
void clearScreen(int channel);
void moveCursorToUpLeft(int channel);
void moveCursor(int channel, int row, int col);
void hideCursor(int channel);
void deleteFromCursorToEol(int channel);
void saveCursor(int channel);
void restoreCursor(int channel);

#endif
