#include <train.h>
#include <syscall.h>
#include <ui.h>
#include <ts7200.h>
#include <parser.h>
#include <trainset.h>
#include <utils.h>

void initializeUI(TrainSetData *data) {
    /* Initialize. */
    clearScreen();
    hideCursor();
    PutStr(COM2, TCS_YELLOW);

    /* Display header. */
    moveCursor(2, 1);
    PutStr(COM2, "    CS 452 Real Time Train Control Station  (by Siqing & Yu Meng)");
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
    PutStr(COM2, TCS_RESET);
    printSwitchTable(data);
    PutStr(COM2, TCS_YELLOW);

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

void printTime() {
    unsigned int time = 0;          // timer
    int idlePercent;                // performance monitor
    int idlePercentLastChecked = 0;
    for (;;) {
        unsigned int newtime = Time() / 10;
        if (newtime > time) {
            time = newtime;
            moveCursor(TIMER_R, TIMER_C);
            displayTime(time);
        }
        Delay(12);
        if (idlePercentLastChecked + 10 < time) {

            idlePercentLastChecked = time;

            idlePercent = IdlePercent();
            if (idlePercent >= 0) {
                moveCursor(PM_R, PM_C);
                Printf(COM2, "%d.%d ", idlePercent/10, idlePercent%10);
            }
        }
    }
}

void pullSensorFeed() {
    /* Sensor */
    TrainSetSensorData data;
    data.numSensorPast = 0;

    for (;;) {
        trainset_subscribeSensorFeeds();
        // Delay(5);
        trainset_pullSensorFeeds(&data);
    }
}

void train() {

    /* Trainset Initialization. */
    TrainSetData trainsetData;
    trainset_init(&trainsetData);
    initializeUI(&trainsetData);

    /* Input Initialization. */
    char inputBuffer[256];
    int inputSize = 0;
    char c;

    Create(1, &printTime);
    Create(2, &pullSensorFeed);

    for ( ;; ) {

        /* READ INPUT & PARSE COMMAND */
        int result = Getc(COM2);
        c = (char) result;

        /* Input available */
        if (c == '\r') {
            Printf(COM2, "\n\r");
            /* Return pressed, parse input */
            inputBuffer[inputSize] = '\0';

            /* Print user input with time stamp. */
            moveCursor(LOG_R, LOG_C);
            deleteFromCursorToEol();
            PutStr(COM2, TCS_CYAN);
            displayTime(Time() / 10);
            Printf(COM2, " : %s%s", inputBuffer, TCS_RESET);
            moveCursor(LOG_R + 1, LOG_C + 4);
            deleteFromCursorToEol();

            result = parseCommand(&trainsetData, inputBuffer);
            switch(result) {
                case CMD_HALT:
                    goto TearDown;
                case CMD_FAILED:
                    Printf(COM2, "%sERROR: Command not recognized!%s", TCS_RED, TCS_RESET);
                    break;
                case CMD_PM_ON:
                    moveCursor(PM_R, PM_C-14);
                    PutStr(COM2, "Idle Percent: ...");
                    TurnMonitor(1);
                    break;
                case CMD_PM_OFF:
                    TurnMonitor(0);
                    moveCursor(PM_R, 0);
                    deleteFromCursorToEol();
                    break;
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
            Putc(COM2, ' ');
        } else {
            moveCursor(CMD_R, CMD_C + inputSize + 1);
            Putc(COM2, c);

           /* Read input into Buffer */
            inputBuffer[inputSize] = c;
            inputSize++;
        }

    } // forever loop

    TearDown:
    trainset_stop();
    moveCursor(END_R, 0);
    ExitProgram();
}
