/*
 * ui.c
 */

#include <ui.h>
#include <bwio.h>
#include <utils.h>

/* Display */

void clearScreen() {
    bwputstr(COM2, "\033[2J");
}

void moveCursorToUpLeft() {
    bwputstr(COM2, "\033[H");
}

void moveCursor(int row, int col) {
    bwprintf(COM2, "\033[%u;%uH", row, col);
}

void hideCursor() {
    bwputstr(COM2, "\033[?25l");
}

void deleteFromCursorToEol() {
    bwputstr(COM2, "\033[K");
}

void saveCursor() {
    bwputstr(COM2, "\033[s");
}

void restoreCursor() {
    bwputstr(COM2, "\033[u");
}

void resetColor() {
    bwputstr(COM2, TCS_RESET);
}
