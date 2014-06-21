#include <train.h>
#include <syscall.h>
#include <ui.h>
#include <ts7200.h>
#include <parser.h>

void initializeUI() {
    /* Initialize. */
    clearScreen();
    hideCursor();
    PutStr(COM2, TCS_YELLOW);

    /* Display header. */
    moveCursor(2, 1);
    PutStr(COM2, "      CS 452 Real Time Train Control Station  (Yu Meng Zhang)");
    moveCursor(3, 1);
    PutStr(COM2, "====================================================================");


    /* Display Timer Frame. */
    moveCursor(TIMER_R, TIMER_C - 15);
    Printf(COM2, "Time Elapsed: ");

    /* Display Switch Table Frame. */
    moveCursor(SWTABLE_R - 2, SWTABLE_C);
    PutStr(COM2, "Switch State Table");
    moveCursor(SWTABLE_R - 1, SWTABLE_C);
    PutStr(COM2, "----------------------------------------------");
    moveCursor(SWTABLE_R + 5, SWTABLE_C);
    PutStr(COM2, "----------------------------------------------");

    /* Display Sensor Table Frame. */
    moveCursor(SENTABLE_R - 1, SENTABLE_C - 6);
    PutStr(COM2, "Sensors Past ");
    moveCursor(SENTABLE_R, SENTABLE_C - 6);
    Printf(COM2, "%sTotal 0:%s", TCS_WHITE, TCS_YELLOW);

    /* Command Frame. */
    moveCursor(CMD_R - 1, CMD_C - 3);
    PutStr(COM2, "Type Your Command: ");
    moveCursor(CMD_R, CMD_C - 2);
    PutStr(COM2, "> ");

    /* Tear Down. */
    PutStr(COM2, TCS_RESET);

}

void displayTime(unsigned int timerCount) {

    int tenthSecond = timerCount;
    int seconds = tenthSecond / 10;
    int minutes = seconds / 60;
    Printf(COM2, "%u'%u.%u",
        minutes,
        seconds % 60,
        tenthSecond % 10);
    deleteFromCursorToEol();
}

void train() {

    initializeUI();

    /* Timer Initialization. */
    unsigned int time;

    /* Input Initialization. */
    char inputBuffer[256];
    int inputSize = 0;
    char c;

    int loopCount = 0;
    for (; ; loopCount = (loopCount + 1) % 5000) {


        /* DISPLAY TIMER */
        unsigned int newtime = Time()/ 10;
        if (newtime == time) {
            continue;
        }
        time = newtime;

        saveCursor();
        moveCursor(TIMER_R, TIMER_C);
        displayTime(time);
        restoreCursor();

        /* READ INPUT & PARSE COMMAND */

        int result = Getc(COM2);
        // Delay(2);
        if (result == 0 ) {
            /* No input yet. */
            continue;
        }

        c = (char) result;

        /* Input available */
        if (c == '\r') {
            Printf(COM2, "\n\r");
            /* return pressed, parse input */
            inputBuffer[inputSize] = '\0';

            moveCursor(LOG_R, LOG_C);
            deleteFromCursorToEol();
            PutStr(COM2, TCS_CYAN);
            displayTime(time);
            Printf(COM2, " : %s%s", inputBuffer, TCS_RESET);
            moveCursor(LOG_R + 1, LOG_C + 4);
            deleteFromCursorToEol();

            result = parseCommand(inputBuffer);
            if (result == CMD_HALT) {
                goto TearDown;
            } else if (result == CMD_FAILED) {
                Printf(COM2, "%sERROR: Command not recognized!%s", TCS_RED, TCS_RESET);
            // } else if (result == CMD_PM_ON) {
            //     pm_show = 1;
            // } else if (result == CMD_PM_OFF) {
            //     moveCursor(PM_R, PM_C);
            //     deleteFromCursorToEol();
            //     pm_max = 0;
            //     pm_show = 0;
            }

            inputSize = 0;
            moveCursor(CMD_R, CMD_C);
            deleteFromCursorToEol();
        } else if (c == '\b') {
            if (inputSize == 0) {
              continue;
            }
            inputSize--;
            moveCursor(CMD_R, CMD_C + inputSize + 1);
            deleteFromCursorToEol();
        } else {
            moveCursor(CMD_R, CMD_C + inputSize + 1);
            Printf(COM2, "%c", c);

           /* Read input into Buffer */
            inputBuffer[inputSize] = c;
            inputSize++;
        }

    } // forever loop

    TearDown:
    moveCursor(END_R, 0);
    ExitProgram();
}
