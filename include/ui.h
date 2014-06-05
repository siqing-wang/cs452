/*
 * ui.h
 * include UI related syntax
 */


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
