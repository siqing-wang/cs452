/*
 * ui.h
 * include UI related syntax
 */

/* Position macros. */
#define TIMER_R     4           // timer
#define TIMER_C     60

#define SWTABLE_R   8           // switch table
#define SWTABLE_C   5
#define SWTABLE_NPERLINE 5

#define SENTABLE_R  16          // sensor table
#define SENTABLE_C  11

#define LOG_R       19          // system log
#define LOG_C       5

#define CMD_R       24          // command
#define CMD_C       7

#define PM_R        1          // performance monitor
#define PM_C        65

#define END_R       30          // end

/* Color Terminal control sequence */
#define TCS_RESET   "\033[0m"
#define TCS_RED     "\033[31m"
#define TCS_BLACK   "\033[30m"
#define TCS_GREEN   "\033[32m"
#define TCS_YELLOW  "\033[33m"
#define TCS_BLUE    "\033[34m"
#define TCS_MAGENTA "\033[35m"
#define TCS_CYAN    "\033[36m"
#define TCS_WHITE   "\033[37m"

void clearScreen();
void moveCursorToUpLeft();
void moveCursor(int row, int col);
void hideCursor();
void deleteFromCursorToEol();
void saveCursor();
void restoreCursor();
void resetColor();
