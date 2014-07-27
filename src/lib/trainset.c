/* trainset.c - All commands related with trainset */

#include <trainset.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <track.h>
#include <train_calibration.h>
#include <train_control.h>
#include <timer.h>

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
        *(cur + 3) = ' ';
        *name = *name + 4;
    } else {
        *(cur + 1) = '1';
        *(cur + 2)= (char) ('0' + num % 10);
        *(cur + 3) = ' ';
        *name = *name + 4;
    }
}

void printSensorTable(TrainSetData *data) {
    int start = 0;
    int numSensorPast = data->totalSensorPast;
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
    PrintfAt(COM2, SENTABLE_R, SENTABLE_C, "%d: %s", numSensorPast, printBuffer);
}

/* execute commands */

void trainset_setSpeed(int train_number, int train_speed) {
    char cmd[2];
    cmd[0] = (char)train_speed;
    cmd[1] = (char)train_number;
    PutSizedStr(COM1, cmd, 2);
    IOidle(COM1);
}

void trainset_reverse(int train_number) {
    char cmd[2];
    cmd[0] = (char)REVERSE;
    cmd[1] = (char)train_number;
    PutSizedStr(COM1, cmd, 2);
    IOidle(COM1);
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
    IOidle(COM1);
}

void trainset_subscribeSensorFeeds() {
    Putc(COM1, (char)SENSOR_SUBSCRIBE_ALL);
    IOidle(COM1);
}

/* Return if table has been changed. */
int trainset_addToSensorTable(TrainSetData *data, int sensorGroup, int sensorNum) {
    track_node *node = getSensorTrackNode(data, sensorGroup, sensorNum);

    if (node->type == NODE_NONE) {
        return 0;
    }
    assert(node->type == NODE_SENSOR, "trainset_addToSensorTable: adding Node that is not a sensor.");

    /* Add to global sensor table */
    data->sentable[data->totalSensorPast % SENTABLE_SIZE] = node;
    data->totalSensorPast = data->totalSensorPast + 1;

    /* Add sensor to corresponding train's data */
    TrainData *trdata;
    Lock *trLock;

    int i = 0;
    for (i = 0; i < TRAIN_NUM; i++) {
        trdata = data->trtable[i];
        trLock = data->trtableLock[i];

        /* Wait for second round */
        if (trdata->init <= 0) {
            continue;
        }

        /* Only accept expect sensor */
        if ((node != trdata->nextSensor) && (node != trdata->nextNextSensor) && (node != trdata->nextWrongSensor)) {
            continue;
        }

        AcquireLock(trLock);

        int timetick = Time();
        if (node == trdata->nextSensor) {
            trdata->estimateTimetickHittingLastSensor = trdata->expectTimetickHittingNextSensor;
        }
        else if (node == trdata->nextNextSensor) {
            /* Broken sensor detected */
            PrintfAt(COM2, LOG_R, LOG_C, "%sSensor %s is broken. Removed from graph.%s%s",TCS_RED, trdata->nextSensor->name, TCS_RESET, TCS_DELETE_TO_EOL);
            Log("Sensor %s is broken.", trdata->nextSensor->name);
            fixBrokenSensor(data, trdata->nextSensor);

            trdata->estimateTimetickHittingLastSensor = trdata->expectTimetickHittingNextNextSensor;
        }
        else {
            /* Broken switch detected */
            PrintfAt(COM2, LOG_R, LOG_C, "%sSwitch %s is broken. Removed from graph.%s%s",TCS_RED, trdata->nextSwitch->name, TCS_RESET, TCS_DELETE_TO_EOL);
            Log("Switch %s is broken.", trdata->nextSwitch->name);
            fixBrokenSwitch(data, trdata->nextSwitch);
        }

        trdata->lastSensor = node;
        trdata->lastLandmark = node;
        trdata->numSensorPast = trdata->numSensorPast + 1;
        trdata->nextSensor = nextSensorOrExit(data, node);
        trdata->nextNextSensor = nextSensorOrExit(data, trdata->nextSensor);
        trdata->nextWrongSensor = nextWrongDirSensorOrExit(data, node);
        trdata->nextSwitch = nextBranchOrExit(data, node);

        /* Calibration */
        if ((trdata->estimateTimetickHittingLastSensor > 0)
            && ((timetick - trdata->actualTimetickHittingLastSensor) > 0)
            && (trdata->timetickSinceSpeedChange > 400)
            && (!trdata->stopInProgress)
            && (!trdata->shortMoveInProgress)) {
            int friction = (int)(node->friction * 700) * (1.0 *
                (trdata->estimateTimetickHittingLastSensor - trdata->actualTimetickHittingLastSensor) /
                (timetick - trdata->actualTimetickHittingLastSensor));
            node->friction = 1.0 * ((int)(node->friction * 300) + friction) / 1000;
        }
        trdata->actualTimetickHittingLastSensor = timetick;

        /* Update location info */
        trdata->timetickWhenHittingSensor = trdata->timetickSinceSpeedChange;
        if ((trdata->needToStop)
            && (trdata->estimateTimetickHittingLastSensor > 0)
            && (trdata->timetickSinceSpeedChange > 400)
            && (!trdata->stopInProgress)
            && (!trdata->shortMoveInProgress)) {
            int diff = trdata->actualTimetickHittingLastSensor - trdata->estimateTimetickHittingLastSensor;
            trdata->delayToStop = trdata->delayToStop + diff;
            Log("Delay + %d ticks at sensor %s", diff, node->name);
        }

        if (trdata->reverse) {
            trdata->distanceAfterLastLandmark = 20;
        }
        else {
            trdata->distanceAfterLastLandmark = 140;
        }

        /* Estimate timetick for next/nextNext sensor */
        int timeInterval = calculate_expectArrivalDuration(trdata, nextSensorDistance(data, node), trdata->nextSensor->friction);
        if (timeInterval < 0) {
            trdata->expectTimetickHittingNextSensor = -1;
        }
        else {
            trdata->expectTimetickHittingNextSensor = trdata->actualTimetickHittingLastSensor + timeInterval;
        }

        timeInterval = calculate_expectArrivalDuration(trdata, nextSensorDistance(data, trdata->nextSensor), trdata->nextNextSensor->friction);
        if ((trdata->expectTimetickHittingNextSensor < 0) || (timeInterval < 0)) {
            trdata->expectTimetickHittingNextNextSensor = -1;
        }
        else {
            trdata->expectTimetickHittingNextNextSensor = trdata->expectTimetickHittingNextSensor + timeInterval;
        }

        ReleaseLock(trLock);

        /* Found an inited train trigger that sensor */
        return 1;
    }

    for (i = 0; i < TRAIN_NUM; i++) {
        trdata = data->trtable[i];
        trLock = data->trtableLock[i];

        if (trdata->init != 0) {
            continue;
        }

        /* For those trains which do not have expected sensor */
        assert((trdata->nextSensor == (track_node *)0), "addToSensorTable : uninit train has next sensor");

        AcquireLock(trLock);

        trdata->lastSensor = node;
        trdata->numSensorPast = trdata->numSensorPast + 1;

        int timetick = Time();

        /* Calibration */
        trdata->actualTimetickHittingLastSensor = timetick;

        /* Update location info */
        trdata->timetickWhenHittingSensor = 0;

        /* Estimate timetick for next/nextNext sensor */
        trdata->expectTimetickHittingNextSensor = -1;
        trdata->expectTimetickHittingNextNextSensor = -1;

        if (node == trdata->lastLandmark) {
            trdata->nextSensor = nextSensorOrExit(data, node);
            trdata->nextNextSensor = nextSensorOrExit(data, trdata->nextSensor);
            trdata->nextWrongSensor = nextWrongDirSensorOrExit(data, node);
            trdata->nextSwitch = nextBranchOrExit(data, node);

            /* Estimate timetick for next/nextNext sensor */
            int timeInterval = calculate_expectArrivalDuration(trdata, nextSensorDistance(data, node), trdata->nextSensor->friction);
            if (timeInterval < 0) {
                trdata->expectTimetickHittingNextSensor = -1;
            }
            else {
                trdata->expectTimetickHittingNextSensor = trdata->actualTimetickHittingLastSensor + timeInterval;
            }

            timeInterval = calculate_expectArrivalDuration(trdata, nextSensorDistance(data, trdata->nextSensor), trdata->nextNextSensor->friction);
            if ((trdata->expectTimetickHittingNextSensor < 0) || (timeInterval < 0)) {
                trdata->expectTimetickHittingNextNextSensor = -1;
            }
            else {
                trdata->expectTimetickHittingNextNextSensor = trdata->expectTimetickHittingNextSensor + timeInterval;
            }

            trdata->init = 1;
        }

        ReleaseLock(trLock);

        /* Found an initing train trigger that sensor */
        return 1;
    }

    /* No train trigger that sensor */
    return 0;
}

int trainset_pullSensorFeeds(TrainSetData *data) {
    int sensorBit = 0;          // 0 (for 1-8) or 1 (for 9-16)
    int sensorGroup = 0;        // ABCDE

    int changed = 0;
    int verified = 0;           // do not redraw graph if no train touch that sensor

    int i;
    char results[10];
    GetSensorData(results);
    for (i = 0; i < 10; i++) {
        if(data->lastByte[i] != results[i]) {
            changed = 1;
        }
        data->lastByte[i] = results[i];
    }
    if (!changed) {
        return 0;               // Sensor not changed
    }
    for (i = 0; i < 10; i++) {
        sensorBit = i % 2;
        sensorGroup = i / 2;
        int bitPosn = 0;
        int result = (int) results[i];
        for ( ; bitPosn < 8; bitPosn++) {
            int mask = 1 << ( 7 - bitPosn );
            if (result & mask) {
                /* Bit is set. */
                verified |= trainset_addToSensorTable(data, sensorGroup, sensorBit * 8 + bitPosn + 1);
            }
        }
    }

    printSensorTable(data);
    return verified;
}

void trainset_go() {
    Putc(COM1, (char)96);
}

void trainset_init(TrainSetData *data) {
    int i = 0;
    for( ; i<TRAIN_NUM; i++) {
        data->trtable[i]->reverse = 0;
        data->trtable[i]->stopInProgress = 0;
        data->trtable[i]->shortMoveInProgress = 0;
        data->trtable[i]->init = -1;

        data->trtable[i]->lastSpeed = 0;
        data->trtable[i]->targetSpeed = 0;
        data->trtable[i]->timetickSinceSpeedChange = 0;
        data->trtable[i]->delayRequiredToAchieveSpeed = 0;

        data->trtable[i]->distanceAfterLastLandmark = 0;
        data->trtable[i]->timetickWhenHittingSensor = 0;

        data->trtable[i]->numSensorPast = 0;
        data->trtable[i]->nextSensor = (track_node *)0;
        data->trtable[i]->estimateTimetickHittingLastSensor = -1;
        data->trtable[i]->actualTimetickHittingLastSensor = 0;
        data->trtable[i]->expectTimetickHittingNextSensor = -1;
        data->trtable[i]->expectTimetickHittingNextNextSensor = -1;

        data->trtable[i]->needToStop = 0;
        data->trtable[i]->delayToStop = 0;
        data->trtable[i]->continueToStop = 0;
        data->trtable[i]->blockedByOthers = 0;
        data->trtable[i]->nextLocation = (track_node *)0;
        data->trtable[i]->nextLocationOffset = 0;
        data->trtable[i]->finalLocation = (track_node *)0;
        data->trtable[i]->finalLocationAlt = (track_node *)0;
        data->trtable[i]->finalLocationOffset = 0;

        data->trtable[i]->needToCleanTrack = 0;
        data->trtable[i]->shortMoveReserved = 0;

        lock_init(data->trtableLock[i]);
    }
    for(i = 0; i < 10; i++) {
        data->lastByte[i] = 0;
    }
    data->totalSensorPast = 0;

    /* Initialize locks. */
    lock_init(data->swtableLock);
    lock_init(data->trackLock);

    int *swtable = data->swtable;

    trainset_go();

    /* Train Speed Table init. */
    i = 40;
    for ( ; i<60; i++) {
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
