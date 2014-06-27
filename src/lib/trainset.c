/* trainset.c - All commands related with trainset */

#include <trainset.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <timer.h>

/* execute commands helpers */
int getSwitchIndex(int switch_number) {
    if (switch_number <= 18) {
        return switch_number - 1;
    } else if (switch_number >= 153 && switch_number <= 156) {
        return switch_number - 153 + 18;
    }
    return -1;
}

int getSwitchNumber(int index) {
    if (index < 18) {
        return index + 1;
    } else if (index < SWITCH_TOTAL) {
        return index - 18 + 153;
    }
    return -1;
}

/* Update a single item in switch table, to minimize output. */
void updateSwitchTable(TrainSetData *data, int switch_number) {
    int switch_index = getSwitchIndex(switch_number);
    int line = switch_index / SWTABLE_NPERLINE;
    int posn = (switch_index % SWTABLE_NPERLINE) * 9;

    char dir;
    if (*(data->swtable + switch_index) == SWITCH_CURVE) {
        dir = 'C';
    } else {
        dir = 'S';
    }
    if (switch_number < 10) {
        PrintfAt(COM2, SWTABLE_R + line, SWTABLE_C + posn, "  %u - %c ", switch_number, dir);
    } else if (switch_number < 100) {
        PrintfAt(COM2, SWTABLE_R + line, SWTABLE_C + posn, " %u - %c ", switch_number, dir);
    } else {
        PrintfAt(COM2, SWTABLE_R + line, SWTABLE_C + posn, "%u - %c ", switch_number, dir);
    }
}

// busy wait print! so only use in initialization
void printSwitchTable(TrainSetData *data) {
    moveCursor(SWTABLE_R, SWTABLE_C);
    int swtable_r = SWTABLE_R;
    /* print header */
    int i = 0;
    deleteFromCursorToEol(COM2);
    for ( ; i < SWITCH_TOTAL; i++) {

        int switch_number = getSwitchNumber(i);
        updateSwitchTable(data, switch_number);

        if ((i + 1) % SWTABLE_NPERLINE == 0) {
            swtable_r++;
            moveCursor(swtable_r, SWTABLE_C);
            deleteFromCursorToEol(COM2);
            IOidle(COM2);       // wait until initialization is done, i.e. IO idle.
        }
    }
}


int sensorNameToInt(int group, int num) {
    return group * 100 + num;
}

void sensorIntToName(int code, char** name) {
    int group = code / 100;
    int num = code % 100;
    assert(num <= 16, "SensorIntToName: Invalid sensor code.");
    char *cur = *name;

    *cur = (char) ('A' + group);
    if (num < 10) {
        *(cur + 1) = (char) ('0' + num);
        *(cur + 2) = ' ';
        *name = *name + 3;
    } else {
        *(cur + 1) = '1';
        *(cur + 2)= (char) ('0' + num % 10);
        *(cur + 3) = ' ';
        *name = *name + 4;
    }
}

void printSensorTable(TrainSetSensorData *data) {
    int start = 0;
    int numSensorPast = data->numSensorPast;
    int numberOfItemsPrinted = numSensorPast;
    int *sentable = data->sentable;

    if (numSensorPast > SENTABLE_SIZE) {
        start = numSensorPast - SENTABLE_SIZE;
        numberOfItemsPrinted = SENTABLE_SIZE;
    }

    int i = start + numberOfItemsPrinted - 1;
    char printBuffer[ 4 * SENTABLE_SIZE+1];
    char *cur = printBuffer;
    for (; i >= start; i--) {
        /* Print backwards. */
        sensorIntToName(*(sentable + i % SENTABLE_SIZE), &cur);

    }
    *cur = '\0';
    PrintfAt(COM2, SENTABLE_R, SENTABLE_C, "%s%d: %s", TCS_DELETE_TO_EOL, numSensorPast, printBuffer);
}

/* execute commands */

void trainset_setSpeed(TrainSetData *data, int train_number, int train_speed) {
    Putc(COM1, (char)train_speed);
    Putc(COM1, (char)train_number);
    *(data->tstable + train_number - 1) = train_speed;
}

void trainset_reverse(TrainSetData *data, int train_number) {
    Putc(COM1, (char)REVERSE);
    Putc(COM1, (char)train_number);
    int train_speed = *(data->tstable + train_number - 1);
    trainset_setSpeed(data, train_number, train_speed);
}

void trainset_turnSwitch(TrainSetData *data, int switch_number, int switch_direction) {
    Putc(COM1, (char)switch_direction);
    Putc(COM1, (char)switch_number);
    Putc(COM1, (char)SWITCH_TURN_OUT);
    *(data->swtable + getSwitchIndex(switch_number)) = switch_direction;
}

void trainset_subscribeSensorFeeds() {
    Putc(COM1, (char)SENSOR_SUBSCRIBE_ALL);
}

/* Return if table has been changed. */
void trainset_addToSensorTable(TrainSetSensorData *data, int sensorGroup, int sensorNum) {
    int sensorCode = sensorNameToInt(sensorGroup, sensorNum);
    int numSensorPast = data->numSensorPast;
    data->sentable[numSensorPast % SENTABLE_SIZE] = sensorCode;
    data->numSensorPast = numSensorPast + 1;
}

int trainset_pullSensorFeeds(TrainSetSensorData *data) {

    int sensorBit = 0;          // 0 (for 1-8) or 1 (for 9-16)
    int sensorGroup = 0;        // ABCDE

    int changed = 0;    // do not reprint if not chanegd

    int i;
    for (i=0; i<10; i++) {
        sensorBit = i % 2;
        sensorGroup = i / 2;
        int bitPosn = 0;
        int result = (int) Getc(COM1);
        if(data->lastByte[i] == result) {
            continue;
        }
        data->lastByte[i] = result;
        changed = 1;
        for ( ; bitPosn < 8; bitPosn++) {
            int mask = 1 << ( 7 - bitPosn );
            if (result & mask) {
                /* Bit is set. */
                trainset_addToSensorTable(data, sensorGroup, sensorBit * 8 + bitPosn + 1);

                // C15
                if ((sensorGroup == 2) && ((sensorBit * 8 + bitPosn + 1) == 15)) {
                    Putc(COM1, (char)0);
                    Putc(COM1, (char)49);
                }

                // // B16
                // if ((sensorGroup == 1) && ((sensorBit * 8 + bitPosn + 1) == 16)) {
                //     data->val = debugTimer_getVal();
                // }
                // // C10
                // if ((sensorGroup == 2) && ((sensorBit * 8 + bitPosn + 1) == 10)) {
                //     int newVal = debugTimer_getVal() - data->val;
                //     data->data = (data->data * data->count + newVal)/(data->count + 1);
                //     data->count = data->count + 1;
                //     PrintfAt(COM2, CMD_R + 3, 1, "new = %d avg = %d count = %d%s\n", newVal, data->data, data->count, TCS_DELETE_TO_EOL);
                //     data->val = 0;
                // }
            }
        }
    }
    if (changed) {
        printSensorTable(data);
    }

    return 1;
}

void trainset_go() {
    Putc(COM1, (char)96);

}

void trainset_init(TrainSetData *data) {

    int *tstable = data->tstable;
    int *swtable = data->swtable;

    trainset_go();

    /* Train Speed Table init. */
    int i=0;
    for ( ; i<80; i++){
        *(tstable+i) = 0;

    }
    i = 40;
    for ( ; i<55; i++) {
        trainset_setSpeed(data, i, 0);
    }


    /* Switch Direction Table */
    i=0;
    for ( ;i <SWITCH_TOTAL; i++){
        if (i == 18 || i == 20 ) {
            *(swtable + i) = SWITCH_STRAIGHT;
        } else {
            *(swtable + i) = SWITCH_CURVE;
        }
        trainset_turnSwitch(data, getSwitchNumber(i), *(swtable + i));
    }

    trainset_turnSwitch(data, 6, SWITCH_STRAIGHT);
    trainset_turnSwitch(data, 7, SWITCH_STRAIGHT);
    trainset_turnSwitch(data, 8, SWITCH_STRAIGHT);
    trainset_turnSwitch(data, 10, SWITCH_STRAIGHT);
    trainset_turnSwitch(data, 15, SWITCH_STRAIGHT);


    Putc(COM1, (char)SENSOR_RESET_MODE_ON);
}

void trainset_stop() {
    Putc(COM1, (char)97);
}