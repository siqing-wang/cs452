/*
 * ui.c
 */

#include <ui.h>
#include <utils.h>
#include <syscall.h>

/* Display */

void clearScreen() {
    PutStr(COM2, "\033[2J");
}

void moveCursorToUpLeft() {
    PutStr(COM2, "\033[H");
}

void moveCursor(int row, int col) {
    Printf(COM2, "\033[%u;%uH", row, col);
}

void hideCursor() {
    PutStr(COM2, "\033[?25l");
}

void deleteFromCursorToEol() {
    PutStr(COM2, "\033[K");
}

void saveCursor() {
    PutStr(COM2, "\033[s");
}

void restoreCursor() {
    PutStr(COM2, "\033[u");
}

void resetColor() {
    PutStr(COM2, TCS_RESET);
}
