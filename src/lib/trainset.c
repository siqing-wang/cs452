/* trainset.c - All commands related with trainset */

#include <trainset.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <timer.h>
#include <track.h>
#include <train_data.h>

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
    if (*(data->swtable + switch_index) == DIR_CURVED) {
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

track_node *getSensorTrackNode(TrainSetData *data, int group, int num) {
    track_node *trackNode = data->trackNode;
    return trackNode + group * 16 + num - 1;
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

void trainset_setSpeed(TrainSetData *data, int train_number, int train_speed) {
    char cmd[2];
    cmd[0] = (char)train_speed;
    cmd[1] = (char)train_number;
    PutSizedStr(COM1, cmd, 2);
    int i = 0;
    for( ; i < TRAIN_NUM ; i++) {
        if (data->tstable[i]->trainNum == train_number) {
            data->tstable[i]->lastSpeed = data->tstable[i]->targetSpeed;
            data->tstable[i]->targetSpeed = train_speed;
            data->tstable[i]->timetick = 0;
            data->tstable[i]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, i);
            break;
        }
    }
}

void trainset_reverse(TrainSetData *data, int train_number) {
    trainset_setSpeed(data, train_number, 0);

    int train_speed = 0;
    int i = 0;
    for( ; i < TRAIN_NUM ; i++) {
        if (data->tstable[i]->trainNum == train_number) {
            Delay(data->tstable[i]->timeRequiredToAchieveSpeed);
            train_speed = data->tstable[i]->lastSpeed;
            break;
        }
    }

    char cmd[2];
    cmd[0] = (char)REVERSE;
    cmd[1] = (char)train_number;
    PutSizedStr(COM1, cmd, 2);
    trainset_setSpeed(data, train_number, train_speed);
}

void trainset_turnSwitch(TrainSetData *data, int switch_number, int switch_direction) {
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
    *(data->swtable + getSwitchIndex(switch_number)) = switch_direction;
}

void trainset_subscribeSensorFeeds() {
    Putc(COM1, (char)SENSOR_SUBSCRIBE_ALL);
}

/* Return if table has been changed. */
void trainset_addToSensorTable(TrainSetData *data, int sensorGroup, int sensorNum) {
    track_node *node = getSensorTrackNode(data, sensorGroup, sensorNum);
    int numSensorPast = data->numSensorPast;

    assert(node->type == NODE_SENSOR, "trainset_addToSensorTable: adding Node that is not a sensor.");

    data->sentable[numSensorPast % SENTABLE_SIZE] = node;
    data->numSensorPast = numSensorPast + 1;

    track_node *nextSensor = nextSensorOrExit(data, node);
    PrintfAt(COM2, SENEXPECT_R, SENEXPECT_C, "%s ", nextSensor->name);
    PrintfAt(COM2, SENLAST_R, SENLAST_C, "%s ", node->name);

    int timetick = Time();
    displayTime(timetick/10, SENLAST_R, SENLAST_C + 12);
    displayTime((data->expectTimetick)/10, SENLAST_R, SENLAST_C + 30);
    int diff = timetick / 10 - data->expectTimetick / 10;
    if (diff < 0) {
        diff = 0 - diff;
    }
    displayTime(diff, SENLAST_R, SENLAST_C + 44);

    int timeInterval = expectSensorArrivalTimeDuration(data, 0, node);
    data->expectTimetick = timetick + timeInterval;
    displayTime((data->expectTimetick)/10, SENEXPECT_R, SENEXPECT_C + 12);
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
        data->lastByte[i] = result;
        changed = 1;
        for ( ; bitPosn < 8; bitPosn++) {
            int mask = 1 << ( 7 - bitPosn );
            if (result & mask) {
                /* Bit is set. */
                trainset_addToSensorTable(data, sensorGroup, sensorBit * 8 + bitPosn + 1);
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
    int i = 0;
    for( ; i<TRAIN_NUM; i++) {
        data->tstable[i]->lastSpeed = 0;
        data->tstable[i]->targetSpeed = 0;
        data->tstable[i]->timetick = 0;
        data->tstable[i]->timeRequiredToAchieveSpeed = 0;
    }
    data->tstable[0]->trainNum = 49;
    int *swtable = data->swtable;

    trainset_go();

    /* Train Speed Table init. */
    i = 40;
    for ( ; i<55; i++) {
        trainset_setSpeed(data, i, 0);
    }

    /* Switch Direction Table */
    i = 0;
    for ( ;i <SWITCH_TOTAL; i++){
        if (i == 18 || i == 20 ) {
            *(swtable + i) = DIR_STRAIGHT;
        } else {
            *(swtable + i) = DIR_CURVED;
        }
        trainset_turnSwitch(data, getSwitchNumber(i), *(swtable + i));
    }

    Putc(COM1, (char)SENSOR_RESET_MODE_ON);
}

void trainset_stop() {
    Putc(COM1, (char)97);
}
