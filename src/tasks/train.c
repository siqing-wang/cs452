#include <train.h>
#include <syscall.h>
#include <ui.h>
#include <ts7200.h>
#include <parser.h>
#include <trainset.h>
#include <utils.h>
#include <track.h>

void drawTrack();

void initializeUI(TrainSetData *data) {
    /* Initialize. */
    clearScreen();
    hideCursor();
    PutStr(COM2, TCS_YELLOW);

    /* Display header. */
    moveCursor(2, 1);
    Printf(COM2, "     CS 452 Real Time Train Control Station  (by Siqing & Yu Meng)");
    moveCursor(3, 1);
    Printf(COM2, "======================================================================");


    /* Display Timer Frame. */
    moveCursor(TIMER_R, TIMER_C - 15);
    Printf(COM2, "Time Elapsed: ");

    /* Display Switch Table Frame. */
    moveCursor(SWTABLE_R - 2, SWTABLE_C);
    Printf(COM2, "Switch Table\n    ----------------------------------------------------");
    moveCursor(SWTABLE_R + 4, SWTABLE_C);
    Printf(COM2, "----------------------------------------------------");
    PutStr(COM2, TCS_RESET);
    printSwitchTable(data);
    PutStr(COM2, TCS_YELLOW);

    /* Display Sensor Table Frame. */
    moveCursor(SENTABLE_R, SWTABLE_C);
    Printf(COM2, "Sensors Past");
    moveCursor(SENTABLE_R, SENTABLE_C - 6);
    Printf(COM2, "%sTotal 0:%s", TCS_WHITE, TCS_YELLOW);
    moveCursor(SENEXPECT_R, SWTABLE_C);
    Printf(COM2, "Expecting                at");
    moveCursor(SENLAST_R, SWTABLE_C);
    Printf(COM2, "Last Sensor         past at          expected          diff");

    /* Command Frame. */
    moveCursor(CMD_R - 1, CMD_C - 3);
    Printf(COM2, "Type Your Command: ");
    moveCursor(CMD_R, CMD_C - 2);
    Printf(COM2, "> ");

    /* Tear Down. */
    PutStr(COM2, TCS_RESET);
}

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

void pullSensorFeed() {
    /* Initialize shared data. */
    TrainSetData *data;
    int parentTid;
    Receive(&parentTid, &data, sizeof(data));
    Reply(parentTid, &parentTid, sizeof(parentTid));

    /* Initialize track*/
    init_tracka(data->trackNode);

    drawTrack();

    /* Sensor */
    int i = 0;
    for(i = 0; i < 10; i++) {
        data->lastByte[i] = 0;
    }

    for (;;) {
        trainset_subscribeSensorFeeds();
        trainset_pullSensorFeeds(data);
        Delay(1);
    }
}

void updateSpeedTable() {
    /* Initialize shared data. */
    TrainSetData *data;
    int parentTid;
    Receive(&parentTid, &data, sizeof(data));
    Reply(parentTid, &parentTid, sizeof(parentTid));

    int i = 0;
    for (;;) {
        for(i = 0; i < TRAIN_NUM; i++) {
            data->tstable[i]->timetick = data->tstable[i]->timetick + 1;
        }
        Delay(1);
    }
}


void drawTrack() {
    moveCursor(TRACK_R, TRACK_C);       Printf(COM2,  "*******************************************************");
    moveCursor(TRACK_R+1, TRACK_C);     Printf(COM2,  "             *     *                                     *");
    moveCursor(TRACK_R+2, TRACK_C);     Printf(COM2,  "************     *   ***********************************   *");
    moveCursor(TRACK_R+3, TRACK_C);     Printf(COM2,  "          *     * *              *         *              * *");
    moveCursor(TRACK_R+4, TRACK_C);     Printf(COM2,  " ********      **                  *  *  *                  **");
    moveCursor(TRACK_R+5, TRACK_C);     Printf(COM2,  "               *                    * * *                    *");
    moveCursor(TRACK_R+6, TRACK_C);     Printf(COM2,  "              *                      ***                      *");
    moveCursor(TRACK_R+7, TRACK_C);     Printf(COM2,  "              *                       *                       *");
    moveCursor(TRACK_R+8, TRACK_C);     Printf(COM2,  "              *                      ***                      *");
    moveCursor(TRACK_R+9, TRACK_C);     Printf(COM2,  "              **                    * * *                    **");
    moveCursor(TRACK_R+10, TRACK_C);    Printf(COM2,  " ********     * *                  *  *  *                  * *");
    moveCursor(TRACK_R+11, TRACK_C);    Printf(COM2,  "         *     *  *              *         *              *  *");
    moveCursor(TRACK_R+12, TRACK_C);    Printf(COM2,  " **********     *    ************************************   *");
    moveCursor(TRACK_R+13, TRACK_C);    Printf(COM2,  "            *     *                                       *");
    moveCursor(TRACK_R+14, TRACK_C);    Printf(COM2,  "**************      *************************************");
    moveCursor(TRACK_R+15, TRACK_C);    Printf(COM2,  "              *            *                     *");
    moveCursor(TRACK_R+16, TRACK_C);    Printf(COM2,  "************************************************************* ");
}

void train() {
    /* Trainset data initialization. */
    TrainSetData trainsetData;
    TrainSpeedData trainSpeedData[TRAIN_NUM];
    int i = 0;
    for(i = 0; i < TRAIN_NUM; i++) {
        trainsetData.tstable[i] = &trainSpeedData[i];
    }
    trainsetData.numSensorPast = 0;
    trainsetData.expectTimetick = 0;
    track_node trackNode[TRACK_MAX];
    trainsetData.trackNode = trackNode;

    /* Trainset Initialization. */
    trainset_init(&trainsetData);
    clearScreen();
    PrintfAt(COM2, 10, 10, "Initializing ... Please wait for a few seconds. ");
    IOidle(COM1);       // wait until initialization is done, i.e. IO idle.

    initializeUI(&trainsetData);
    IOidle(COM2);       // wait until initialization is done, i.e. IO idle.

    /* Input Initialization. */
    char inputBuffer[256];
    int inputSize = 0;
    char c;

    /* Create Children tasks. */
    Create(1, &printTime);

    int childTid1 = Create(2, &pullSensorFeed);
    int childTid2 = Create(2, &updateSpeedTable);
    int sendResult;
    TrainSetData *dataPtr = &trainsetData;
    Send(childTid1, &dataPtr, sizeof(dataPtr), &sendResult, sizeof(sendResult));
    Send(childTid2, &dataPtr, sizeof(dataPtr), &sendResult, sizeof(sendResult));

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
            Printf(COM2, ": %s%s%s", inputBuffer, TCS_RESET, TCS_DELETE_TO_EOL);
            moveCursor2(LOG_R + 1, LOG_C + 4);
            deleteFromCursorToEol();

            result = parseCommand(&trainsetData, inputBuffer);
            switch(result) {
                case CMD_HALT:
                    goto TearDown;
                case CMD_FAILED:
                    Printf(COM2, "%sERROR: Command not recognized!%s", TCS_RED, TCS_RESET);
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
    trainset_stop();
    moveCursor2(END_R, 0);

    i = 0;
    Printf(COM2, "Restriction factor \n");
    for( ; i < TRACK_MAX ; i++) {
        track_node *node= (track_node *)(trainsetData.trackNode + i);
        Printf(COM2, "%d:%d ", i, (int)(1000 *node->restriction));
        if ((i % 10 == 0) && (i != 0)) {
            Printf(COM2, "\n");
        }
    }


    IOidle(COM1);
    IOidle(COM2);
    ExitProgram();
}
