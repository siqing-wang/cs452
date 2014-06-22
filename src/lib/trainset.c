/* trainset.c - All commands related with trainset */

#include <trainset.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>

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
    moveCursor(SWTABLE_R + line, SWTABLE_C + posn);

    char dir;
    if (*(data->swtable + switch_index) == SWITCH_CURVE) {
        dir = 'C';
    } else {
        dir = 'S';
    }
    if (switch_number < 10) {
        PutStr(COM2, "  ");
    } else if (switch_number < 100) {
        PutStr(COM2, " ");
    }
    /* TODO: can avoid reprint switch number. */
    Printf(COM2, "%u - %c ", switch_number, dir);
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
        }
    }
}


int sensorNameToInt(int group, int num) {
    return group * 100 + num;
}

void sensorIntToName(int code, char* name) {
    int group = code / 100;
    int num = code % 100;
    assert(num <= 16, "SensorIntToName: Invalid sensor code.");
    *name = (char) ('A' + group);
    if (num < 10) {
        *(name + 1) = (char) ('0' + num);
        *(name + 2) = '\0';
    } else {
        *(name + 1) = '1';
        *(name + 2) = (char) ('0' + num % 10);
        *(name + 3) = '\0';
    }

}

void printSensorTable(TrainSetSensorData *data) {
    saveCursor(COM2);
    moveCursor(SENTABLE_R, SENTABLE_C);
    deleteFromCursorToEol();

    int start = 0;
    int numSensorPast = data->numSensorPast;
    int numberOfItemsPrinted = numSensorPast;
    int *sentable = data->sentable;

    if (numSensorPast > SENTABLE_SIZE) {
        start = numSensorPast - SENTABLE_SIZE;
        numberOfItemsPrinted = SENTABLE_SIZE;
    }

    Printf(COM2, "%d: ", numSensorPast);

    int i = start + numberOfItemsPrinted - 1;
    for (; i >= start; i--) {
        char sensorName[5];
        sensorIntToName(*(sentable + i % SENTABLE_SIZE), sensorName);
        Printf(COM2, "%s ", sensorName);

    }
    restoreCursor(COM2);
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
int trainset_addToSensorTable(TrainSetSensorData *data, int sensorGroup, int sensorNum) {
    int sensorCode = sensorNameToInt(sensorGroup, sensorNum);
    int numSensorPast = data->numSensorPast;
    int lastSensorIndex = (numSensorPast - 1) % SENTABLE_SIZE;
    int lastSensorCode = *(data->sentable + lastSensorIndex);
    if (numSensorPast > 0 && lastSensorCode == sensorCode) {
        /* Same as last sensor code */
        return 0;
    }
    data->sentable[numSensorPast % SENTABLE_SIZE] = sensorCode;
    data->numSensorPast = numSensorPast + 1;
    return 1;
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
        for ( ; bitPosn < 8; bitPosn++) {
            int mask = 1 << ( 7 - bitPosn );
            if (result & mask) {
                /* Bit is set. */
                changed = trainset_addToSensorTable(data, sensorGroup, sensorBit * 8 + bitPosn + 1);
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

    Putc(COM1, (char)SENSOR_RESET_MODE_ON);
}

void trainset_stop() {
    Putc(COM1, (char)97);
}
