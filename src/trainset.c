/* trainset.c - All commands related with trainset */

#include <trainset.h>

static int* tstable;
static int* swtable;
static int* sentable;

static int sensorGroup; // ABCDE
static int sensorBit;   // 0 or 1
static int numSensorPast;

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
void updateSwitchTable(int switch_number) {
    int switch_index = getSwitchIndex(switch_number);
    int line = switch_index / SWTABLE_NPERLINE;
    int posn = (switch_index % SWTABLE_NPERLINE) * 9;
    moveCursor(COM2, SWTABLE_R + line, SWTABLE_C + posn);

    char dir;
    if (*(swtable + switch_index) == SWITCH_CURVE) {
        dir = 'C';
    } else {
        dir = 'S';
    }
    if (switch_number < 10) {
        putstr(COM2, "  ");
    } else if (switch_number < 100) {
        putstr(COM2, " ");
    }
    printf(COM2, "%u - %c ", switch_number, dir);
}

// busy wait print! so only use in initialization
void bwprintSwitchTable() {
    moveCursor(COM2, SWTABLE_R, SWTABLE_C);
    int swtable_r = SWTABLE_R;
    /* print header */
    int i = 0;
    deleteFromCursorToEol(COM2);
    for ( ; i < SWITCH_TOTAL; i++) {

        int switch_number = getSwitchNumber(i);
        updateSwitchTable(switch_number);

        while(ob_print(COM2));

        if ((i + 1) % SWTABLE_NPERLINE == 0) {
            swtable_r++;
            moveCursor(COM2, swtable_r, SWTABLE_C);
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
    assert(num <= 16);
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

void printSensorTable() {
    saveCursor(COM2);
    moveCursor(COM2, SENTABLE_R, SENTABLE_C);

    int start = 0;
    int numberOfItemsPrinted = numSensorPast;
    if (numSensorPast > SENTABLE_SIZE) {
        start = numSensorPast - SENTABLE_SIZE;
        numberOfItemsPrinted = SENTABLE_SIZE;
    }

    printf(COM2, "%d: ", numSensorPast);
    int i = start + numberOfItemsPrinted - 1;
    for (; i >= start; i--) {
        char sensorName[5];
        sensorIntToName(*(sentable + i % SENTABLE_SIZE), sensorName);
        printf(COM2, "%s ", sensorName);

    }
    restoreCursor(COM2);
}

/* execute commands */

void trainset_setSpeed(int train_number, int train_speed) {
    putc(COM1, (char)train_speed);
    putc(COM1, (char)train_number);
    *(tstable + train_number - 1) = train_speed;
}

void trainset_reverse(int train_number) {
    putc(COM1, (char)REVERSE);
    putc(COM1, (char)train_number);
    int train_speed = *(tstable + train_number - 1);
    trainset_setSpeed(train_number, train_speed);
}

void trainset_turnSwitch(int switch_number, int switch_direction) {
    putc(COM1, (char)switch_direction);
    putc(COM1, (char)switch_number);
    putc(COM1, (char)SWITCH_TURN_OUT);
    *(swtable + getSwitchIndex(switch_number)) = switch_direction;
}

void trainset_subscribeSensorFeeds() {
    putc(COM1, (char)SENSOR_SUBSCRIBE_ALL);
}

int trainset_pullSensorFeeds() {
    int result = (int) getc(COM1);

    if (result == -1) {
        // No feed
        return 0;
    } 

    int bitPosn = 0;
    for ( ; bitPosn < 8; bitPosn++) {
        int mask = 1 << ( 7 - bitPosn );
        if (result & mask) {
            // Bit is set.
            int sensorCode = sensorNameToInt(sensorGroup, sensorBit * 8 + bitPosn + 1);
            int lastSensorCode = *(sentable + (numSensorPast - 1) % SENTABLE_SIZE);
            if (numSensorPast > 0 && lastSensorCode == sensorCode) {
                /* Same as last sensor code */
                continue;
            }
            sentable[numSensorPast % SENTABLE_SIZE] = sensorCode;
            numSensorPast++;
            printSensorTable();
        }
    }

    sensorBit++;
    if (sensorBit > 1) {
        sensorBit = 0;
        sensorGroup = (sensorGroup + 1) % 5;
    }
    return 1;
}

void trainset_go() {
    putc(COM1, (char)96);

}

void trainset_init(int *trainSpeedTable, int *switchTable, int *sensorTable) {
    trainset_go();

    /* Train Speed Table init. */
    tstable = trainSpeedTable;

    int i=0;
    for ( ; i<80; i++){
        *(tstable+i) = 0;

    }
    i = 40;
    for ( ; i<55; i++) {
        trainset_setSpeed(i, 0);
        while(ob_print(COM1));
    }


    /* Switch Direction Table */
    swtable = switchTable;

    i=0;
    for ( ;i <SWITCH_TOTAL; i++){
        if (i == 18 || i == 20 ) {
            *(swtable + i) = SWITCH_STRAIGHT;
        } else {
            *(swtable + i) = SWITCH_CURVE;
        }
        trainset_turnSwitch(getSwitchNumber(i), *(swtable + i));
    }

    /* Sensor */
    sentable = sensorTable;
    sensorGroup = 0; // ABCDE
    sensorBit = 0;   // 0 or 1
    numSensorPast = 0;
    getc(COM1); // pull left over if any
    putc(COM1, (char)SENSOR_RESET_MODE_ON);
}

void trainset_stop() {
    putc(COM1, (char)97);
}
