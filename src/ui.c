/* ui.c - User Interface API */

#include <ui.h>

/* Timer */

void displayTime(int channel, int timerCount) {
    int tenthSecond = timerCount;
    int seconds = tenthSecond / 10;
    int minutes = seconds / 60;
    printf(channel, "%u'%u.%u", 
        minutes,
        seconds % 60,
        tenthSecond % 10);
}

/* Display */

void clearScreen(int channel) {
    putstr(channel, "\033[2J");
}

void moveCursorToUpLeft(int channel) {
    putstr(channel, "\033[H");
}

void moveCursor(int channel, int row, int col) {
    printf(channel, "\033[%u;%uH", row, col);
}

void hideCursor(int channel) {
    putstr(channel, "\033[?25l");
}

void deleteFromCursorToEol(int channel) {
    putstr(channel, "\033[K");
}

void saveCursor(int channel) {
    putstr(channel, "\033[s");
}

void restoreCursor(int channel) {
    putstr(channel, "\033[u");
}
