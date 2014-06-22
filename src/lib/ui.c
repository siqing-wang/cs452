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

void moveCursor2(int row, int col) {
    char buf[30];
    char tmp[12];

    buf[0] = '\033';
    buf[1] = '[';
    int size = 2;

    ui2a(row, 10, tmp);
    size += putwToBuffer(buf + size, 0, 0, tmp);

    buf[size] = ';';
    size ++;

    ui2a(col, 10, tmp);
    size += putwToBuffer(buf + size, 0, 0, tmp);

    buf[size] = 'H';
    size ++;

    buf[size] = '\0';
    size ++;
    PutStr(COM2, buf);
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
