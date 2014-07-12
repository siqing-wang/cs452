/* trainset.c - All commands related with trainset */

#include <trainset.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <track.h>
#include <train_calibration.h>

/* For train control. */
#define STOP 0
#define REVERSE 15

/* for switches */
#define SWITCH_TURN_OUT 32
#define SWITCH_STRAIGHT 33
#define SWITCH_CURVE 34


/* execute commands helpers */
int getSwitchIndex(int switch_number) {
    if (switch_number <= 18) {
        return switch_number - 1;
    } else if (switch_number >= 153 && switch_number <= 156) {
        return switch_number - 153 + 18;
    }
    warning("getSwitchIndex: invalid switch number");
    return -1;
}

int getSwitchNumber(int index) {
    if (index < 18) {
        return index + 1;
    } else if (index < SWITCH_TOTAL) {
        return index - 18 + 153;
    }
    warning("getSwitchNumber: invalid index");
    return -1;
}

/* Update a single item in switch table, to minimize output. */
void updateSwitchTable(int switch_number, int switch_direction) {
    int switch_index = getSwitchIndex(switch_number);
    int line = switch_index / SWTABLE_NPERLINE;
    int posn = (switch_index % SWTABLE_NPERLINE) * 9;

    char dir;
    if (switch_direction == DIR_CURVED) {
        dir = 'C';
    } else {
        dir = 'S';
    }
    if (switch_number < 10) {
        posn += 2;
    } else if (switch_number < 100) {
        posn += 1;
    }
    PrintfAt(COM2, SWTABLE_R + line, SWTABLE_C + posn, "%u - %c ", switch_number, dir);

}

// busy wait print! so only use in initialization
void printSwitchTable(TrainSetData *data) {
    moveCursor(SWTABLE_R, SWTABLE_C);
    int swtable_r = SWTABLE_R;
    /* print header */
    int i = 0;
    deleteFromCursorToEol(COM2);

    int switch_number, switch_direction;
    for ( ; i < SWITCH_TOTAL; i++) {
        switch_number = getSwitchNumber(i);
        switch_direction = *(data->swtable + i);
        updateSwitchTable(switch_number, switch_direction);

        if ((i + 1) % SWTABLE_NPERLINE == 0) {
            swtable_r++;
            moveCursor(swtable_r, SWTABLE_C);
            deleteFromCursorToEol(COM2);
            IOidle(COM2);       // wait until initialization is done, i.e. IO idle.
        }
    }
}

track_node *getSensorTrackNode(TrainSetData *data, int group, int num) {
    track_node *track = data->track;
    return track + group * 16 + num - 1;
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

void printSensorTable(TrainSetData *data) {
    int start = 0;
    int numSensorPast = data->numSensorPast;
    int numItemsToPrint = numSensorPast;
    track_node **sentable = data->sentable;

    if (numSensorPast > SENTABLE_SIZE) {
        start = numSensorPast - SENTABLE_SIZE;
        numItemsToPrint = SENTABLE_SIZE;
    }

    int i = start + numItemsToPrint - 1;
    char printBuffer[4 * SENTABLE_SIZE + 1];
    char *cur = printBuffer;

    for (; i >= start; i--) {
        /* Print backwards. */
        track_node *node = *(sentable + i % SENTABLE_SIZE);
        int nodeNameLen = stringLen(node->name);
        memcopy(cur, node->name, nodeNameLen);
        *(cur + nodeNameLen) = ' ';
        cur = cur + nodeNameLen + 1;    // +1 for \0 because \0 not included in string length.
    }
    *cur = '\0';
    PrintfAt(COM2, SENTABLE_R, SENTABLE_C, "%s%d: %s", TCS_DELETE_TO_EOL, numSensorPast, printBuffer);
}

/* execute commands */

void trainset_setSpeed(int train_number, int train_speed) {
    char cmd[2];
    cmd[0] = (char)train_speed;
    cmd[1] = (char)train_number;
    PutSizedStr(COM1, cmd, 2);
}

void trainset_reverse(int train_number) {
    char cmd[2];
    cmd[0] = (char)REVERSE;
    cmd[1] = (char)train_number;
    PutSizedStr(COM1, cmd, 2);
}

void trainset_turnSwitch(int switch_number, int switch_direction) {
    char cmd[4];
    if (switch_direction == DIR_STRAIGHT) {
        cmd[0] = (char)SWITCH_STRAIGHT;
    } else if (switch_direction == DIR_CURVED) {
        cmd[0] = (char)SWITCH_CURVE;
    } else {
        cmd[0] = (char)SWITCH_STRAIGHT;
        warning("trainset_turnSwitch: Invalid switch direction, using straight instead.");
    }
    cmd[1] = (char)switch_number;
    cmd[2] = (char)SWITCH_TURN_OUT;
    PutSizedStr(COM1, cmd, 3);
}

void trainset_subscribeSensorFeeds() {
    Putc(COM1, (char)SENSOR_SUBSCRIBE_ALL);
}

/* Return if table has been changed. */
void trainset_addToSensorTable(TrainSetData *data, int sensorGroup, int sensorNum) {
    track_node *node = getSensorTrackNode(data, sensorGroup, sensorNum);

    assert(node->type == NODE_SENSOR, "trainset_addToSensorTable: adding Node that is not a sensor.");

    /* Not expected sensor */
    if ((data->init > 0) && (node->num != data->expectNextSensorNum) && (node->num != data->expectNextNextSensorNum)) {
        return;
    }

    data->sentable[data->numSensorPast % SENTABLE_SIZE] = node;
    data->numSensorPast = data->numSensorPast + 1;

    if (data->init == 0) {
        return;
    }

    track_node *nextSensor = nextSensorOrExit(data, node);
    track_node *nextNextSensor = nextSensorOrExit(data, nextSensor);
    PrintfAt(COM2, SENEXPECT_R, SENEXPECT_C, "%s ", nextSensor->name);
    PrintfAt(COM2, SENLAST_R, SENLAST_C, "%s ", node->name);

    /* Train Calibration Monitor. */
    int timetick = Time();
    int timetickDiff = 0;   // actual - expect
    if (node->num == data->expectNextSensorNum) {
        if (((timetick - data->lastTimetick) > 0) && ((data->expectNextTimetick - data->lastTimetick) > 0)) {
            timetickDiff = timetick - data->expectNextTimetick;
            int friction = (int)(node->friction * 700) * (1.0 * (data->expectNextTimetick - data->lastTimetick) / (timetick - data->lastTimetick));
            node->friction = 1.0 * ((int)(node->friction * 300) + friction) / 1000;
        }
    }
    else if (node->num == data->expectNextNextSensorNum) {
        if (((timetick - data->lastTimetick) > 0) && ((data->expectNextNextTimetick - data->lastTimetick) > 0)) {
            timetickDiff = timetick - data->expectNextNextTimetick;
            int friction = (int)(node->friction * 700) * (1.0 * (data->expectNextNextTimetick - data->lastTimetick) / (timetick - data->lastTimetick));
            node->friction = 1.0 * ((int)(node->friction * 300) + friction) / 1000;
        }
    }

    data->expectNextSensorNum = nextSensor->num;
    data->expectNextNextSensorNum = nextNextSensor->num;
    data->lastTimetick = timetick;

    int i;
    for (i = 0; i < TRAIN_NUM; i++) {
        AcquireLock(data->tstableLock[i]);
        data->tstable[i]->timetickWhenHittingSensor = data->tstable[i]->timetick;
        data->tstable[i]->lastSpeedDuration = 0;
        data->tstable[i]->delayToStop = data->tstable[i]->delayToStop + timetickDiff;
        ReleaseLock(data->tstableLock[i]);
    }

    displayTime(timetick/10, SENLAST_R, SENLAST_C + 16);                // Display current time hitting this sensor
    if (data->expectNextTimetick < 0) {
        PrintfAt(COM2, SENLAST_R, SENLAST_C + 34, "INFI   ");           // Display expected time for hitting this sensor
        PrintfAt(COM2, SENLAST_R, SENLAST_C + 48, "INFI%s", TCS_DELETE_TO_EOL);     // Their difference
    }
    else {
        displayTime((data->expectNextTimetick)/10, SENLAST_R, SENLAST_C + 34);  // Display expected time for hitting this sensor
        int diff = (timetick - data->expectNextTimetick);
        if (diff < 0) {
            diff = 0 - diff;
        }
        PrintfAt(COM2, SENLAST_R, SENLAST_C + 48, "%d%s", diff, TCS_DELETE_TO_EOL); // Their difference
    }

    int timeInterval = expectSensorArrivalTimeDuration(data, 0, node, nextSensor->friction);
    if (timeInterval < 0) {
        data->expectNextTimetick = -1;
        PrintfAt(COM2, SENEXPECT_R, SENEXPECT_C + 16, "INFI   ");               // Display expected time for hitting next sensor
    }
    else {
        data->expectNextTimetick = timetick + timeInterval;
        displayTime((data->expectNextTimetick)/10, SENEXPECT_R, SENEXPECT_C + 16);  // Display expected time for hitting next sensor
    }

    timeInterval = expectSensorArrivalTimeDuration(data, 0, nextSensor, nextNextSensor->friction);
    if (timeInterval < 0) {
        data->expectNextNextTimetick = -1;
    }
    else {
        data->expectNextNextTimetick = data->expectNextTimetick + timeInterval;
    }
}

int trainset_pullSensorFeeds(TrainSetData *data) {
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
        if (data->init < 0) {
            continue;
        }
        data->lastByte[i] = result;
        for ( ; bitPosn < 8; bitPosn++) {
            int mask = 1 << ( 7 - bitPosn );
            if (result & mask) {
                /* Bit is set. */
                changed = 1;
                trainset_addToSensorTable(data, sensorGroup, sensorBit * 8 + bitPosn + 1);
            }
        }
    }
    if (changed) {
        printSensorTable(data);
    }

    return changed;
}

void trainset_go() {
    Putc(COM1, (char)96);
}

void trainset_init(TrainSetData *data) {
    int i = 0;
    for( ; i<TRAIN_NUM; i++) {
        data->tstable[i]->lastSpeed = 0;
        data->tstable[i]->targetSpeed = 0;
        data->tstable[i]->reverse = 0;
        data->tstable[i]->distance = 0;
        data->tstable[i]->timetick = 0;
        data->tstable[i]->timeRequiredToAchieveSpeed = 0;
        data->tstable[i]->timetickWhenHittingSensor = 0;
        data->tstable[i]->lastSpeedDuration = 0;
        data->tstable[i]->delayToStop = 0;
        data->tstable[i]->needToStop = 0;
        lock_init(data->tstableLock[i]);
    }
    for(i = 0; i < 10; i++) {
        data->lastByte[i] = 0;
    }
    data->numSensorPast = 0;
    data->expectNextTimetick = 0;
    data->expectNextSensorNum = 7;  // A8
    data->expectNextNextSensorNum = 0;
    data->expectNextNextSensorNum = 38;  // C7
    data->lastTimetick = 0;
    data->init = -1;

    /* Initialize locks. */
    lock_init(data->swtableLock);

    int *swtable = data->swtable;

    trainset_go();

    /* Train Speed Table init. */
    i = 40;
    for ( ; i<55; i++) {
        trainset_setSpeed(i, 0);
    }

    /* Switch Direction Table */
    i = 0;
    for ( ;i <SWITCH_TOTAL; i++){
        if (i == 18 || i == 20 ) {
            *(swtable + i) = DIR_STRAIGHT;
        } else {
            *(swtable + i) = DIR_CURVED;
        }
        trainset_turnSwitch(getSwitchNumber(i), *(swtable + i));
    }

    Putc(COM1, (char)SENSOR_RESET_MODE_ON);
}

void trainset_stop() {
    Putc(COM1, (char)97);
}
