#include <train.h>
#include <syscall.h>
#include <ui.h>
#include <ts7200.h>
#include <parser.h>
#include <utils.h>
#include <train_control.h>

void printTime() {
    unsigned int time = 0;          // timer
    int idlePercent;                // performance monitor
    int idlePercentLastChecked = 0;
    for (;;) {
        unsigned int newtime = Time() / 10;
        if (newtime > time) {
            time = newtime;
            displayTime(time, TIMER_R, TIMER_C);
        }
        Delay(12);
        if (idlePercentLastChecked + 10 < time) {

            idlePercentLastChecked = time;

            idlePercent = IdlePercent();
            if (idlePercent >= 0) {
                PrintfAt(COM2, PM_R, PM_C, "%d.%d ", idlePercent/10, idlePercent%10);
            }
        }
    }
}

void train() {
    /* Screen Initialization. */
    clearScreen();
    PrintfAt(COM2, 10, 10, "Initializing ... Please wait for a few seconds. ");

    /* Create Children tasks. */
    int msg = 0;
    int trainCtrlTid = Create(4, &trainControlServer);
    Send(trainCtrlTid, &msg, sizeof(msg), &msg, sizeof(msg));

    Create(1, &printTime);

    /* Input Initialization. */
    char inputBuffer[256];
    int inputSize = 0;
    char c;

    for ( ;; ) {

        /* READ INPUT & PARSE COMMAND */
        int result = Getc(COM2);
        c = (char) result;

        /* Input available */
        if (c == '\r') {
            Printf(COM2, "\n\r");
            /* Return pressed, parse input */
            inputBuffer[inputSize] = '\0';

            inputSize = 0;
            moveCursor2(CMD_R, CMD_C);
            deleteFromCursorToEol();

            /* Print user input with time stamp. */
            PutStr(COM2, TCS_CYAN);
            displayTime(Time() / 10, LOG_R, LOG_C);
            moveCursor2(LOG_R, LOG_C + 7);
            Printf(COM2, ": %s", inputBuffer);
            PutStr(COM2, TCS_RESET);
            PutStr(COM2, TCS_DELETE_TO_EOL);

            PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%s", TCS_DELETE_TO_EOL);

            result = parseCommand(trainCtrlTid, inputBuffer);
            switch(result) {
                case CMD_HALT:
                    goto TearDown;
                case CMD_FAILED:
                    PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sERROR: Command not recognized!%s", TCS_RED, TCS_RESET);
                    break;
                case CMD_PM_ON:
                    PrintfAt(COM2, PM_R, PM_C-14, "Idle Percent: ...");
                    TurnMonitor(1);
                    break;
                case CMD_PM_OFF:
                    TurnMonitor(0);
                    PrintfAt(COM2, PM_R, 0, "%s", TCS_DELETE_TO_EOL);
                    break;
            }


        } else if (c == '\b') {
            if (inputSize == 0) {
              continue;
            }
            inputSize--;
            moveCursor2(CMD_R, CMD_C + inputSize + 1);
            Putc(COM2, ' ');
        } else {
            moveCursor2(CMD_R, CMD_C + inputSize + 1);
            Putc(COM2, c);

           /* Read input into Buffer */
            inputBuffer[inputSize] = c;
            inputSize++;
        }

    } // forever loop

    TearDown:
    moveCursor2(END_R, 0);
    IOidle(COM1);
    IOidle(COM2);
    ExitProgram();
}
