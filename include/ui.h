/*
 * ui.h
 * include UI related syntax
 */

/* Position macros. */
#define TIMER_R     4           // timer
#define TIMER_C     60

#define SWTABLE_R   7           // switch table
#define SWTABLE_C   5
#define SWTABLE_NPERLINE 6

#define SENTABLE_R  12          // sensor table
#define SENTABLE_C  27

#define TR_R                    (SENTABLE_R+2)
#define TR_C                    5
#define TRSPEED_C               (TR_C+17)
#define TRLOCATION_SENSOR_C     (TR_C+32)
#define TRLOCATION_OFFSET_C     (TR_C+42)

#define SENEXPECT_R     		(TR_R+1)          // next sensor expected
#define SENEXPECT_C     		(SENTABLE_C-10)

#define SENLAST_R       		(SENEXPECT_R+1)          // next sensor expected
#define SENLAST_C       		SENEXPECT_C

#define TRACK_R     24
#define TRACK_C     5

#define CMD_R       (TRACK_R+19)          // command
#define CMD_C       7

#define LOG_R       (CMD_R+2)          // system log
#define LOG_C       5

#define SIDELOG_R   2
#define SIDELOG_C   75
#define SIDELOG_HEIGHT (CMD_R+1 - SIDELOG_R)

#define PM_R        1          // performance monitor
#define PM_C        65



#define END_R       54          // end

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

#define TCS_DELETE_TO_EOL "\033[K"

void clearScreen();
void moveCursorToUpLeft();
void moveCursor(int row, int col);
void moveCursor2(int row, int col);
void hideCursor();
void deleteFromCursorToEol();
void saveCursor();
void restoreCursor();
void resetColor();
