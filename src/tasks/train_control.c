 /*
  * train_control.c
  */

#include <train_control.h>
#include <syscall.h>
#include <event.h>
#include <utils.h>
#include <trainset.h>
#include <track.h>
#include <track_graph.h>
#include <train.h>
#include <train_calibration.h>

// A or B
#define TRACK_USED 'A'

void initializeUI(TrainSetData *data);
void sentTo(int destTid, TrainControlMessage *message, CouriersStatus *courierStatus);
void updateTrainSpeed(TrainSetData *data, int trainIndex, int newSpeed);
int findRoute(struct TrainSetData *data, int trainIndex, track_node *start, int startOffset, char *location, int endOffset);

void courier() {
    int serverTid;
    int msg = 0;

    serverTid = WhoIs("Train Control Server");
    assert(serverTid > 0, "courier: invalid train control server tid.");

    TrainControlMessage message;
    for (;;) {
        /* Semd message to control server to announce it is freed and waiting for message */
        message.type = TRAINCTRL_COURIER_FREE;
        Send(serverTid, &message, sizeof(message), &message, sizeof(message));

        /* Send message to tasks */
        Send(message.destTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void switchTask() {
    int serverTid, courierTid;
    int msg = 0;
    TrainSetData *data;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    for(;;) {
        Receive(&courierTid, &message, sizeof(message));
        Reply(courierTid, &msg, sizeof(msg));

        assert(message.type == TRAINCTRL_SW_CHANGE, "switchTask: Invalid message type.");

        if (message.data != *(data->swtable + getSwitchIndex(message.num))) {
            /* Update switch table. */
            AcquireLock(data->swtableLock);
            *(data->swtable + getSwitchIndex(message.num)) = message.data;
            ReleaseLock(data->swtableLock);
            /* Send message to train set to actually turn switch. */
            trainset_turnSwitch(message.num, message.data);
            /* Update terminal switch table. */
            updateSwitchTable(message.num, message.data);
            /* Update realtime graph. */
            trackGraph_turnSw(data, message.num, message.data);
        }

    }
}


void sensorTask() {
    int serverTid;
    int msg = 0;
    TrainSetData *data;
    track_node *lastNode, *nextSensor;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    message.type = TRAINCTRL_SEN_TRIGGERED;
    for (;;) {
        trainset_subscribeSensorFeeds();
        if (trainset_pullSensorFeeds(data)) {
            AcquireLock(data->tstableLock[0]);
            lastNode = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
            nextSensor = nextSensorOrExit(data, lastNode);
            data->tstable[0]->distance = calculate_expectTravelledDistance(data, 0, nextSensor->friction);
            ReleaseLock(data->tstableLock[0]);

            /* Update terminal track graph. */
            if (data->numSensorPast > 1) {
                lastNode = data->sentable[(data->numSensorPast - 2) % SENTABLE_SIZE];
                trackGraph_unhighlightSenPath(data, lastNode);
            }
            lastNode = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
            trackGraph_highlightSenPath(data, lastNode);
        }
        Delay(TIME_INTERVAL);
    }
}

void trainTask() {
    int serverTid, courierTid;
    int trainIndex;
    int msg = 0;
    int speed = 0;
    int delay = 0;
    int result = 0;
    TrainSetData *data;
    track_node *node;
    Lock *tsLock;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    Receive(&serverTid, &trainIndex, sizeof(trainIndex));
    Reply(serverTid, &msg, sizeof(msg));

    tsLock = data->tstableLock[trainIndex];

    TrainControlMessage message;
    for(;;) {
        Receive(&courierTid, &message, sizeof(message));
        Reply(courierTid, &msg, sizeof(msg));

        switch (message.type) {
            case TRAINCTRL_INIT:
                data->init = data->init < 0 ? 0 : data->init;
                data->tstable[trainIndex]->trainNum = message.num;
                PrintfAt(COM2, TR_R, TR_C, "Train%d  Speed :     Location :         +     cm", message.num);

                /* Initialization movement. */
                trainset_setSpeed(message.num, 2);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "2 ", message.data);
                Delay(100);
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "0 ", message.data);
                Delay(100);

                if (data->numSensorPast > 0) {
                    node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
                    // B8, B10, B12
                    if ((node->num = 23) || (node->num = 25) || (node->num = 27)) {
                        trainset_setSpeed(message.num, 15);
                    }
                }
                data->init = 1;
                break;
            case TRAINCTRL_TR_SETSPEED:
                AcquireLock(tsLock);
                updateTrainSpeed(data, trainIndex, message.data);
                ReleaseLock(tsLock);
                trainset_setSpeed(message.num, message.data);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "%d ", message.data);
                break;
            case TRAINCTRL_TR_REVERSE:
                speed = data->tstable[trainIndex]->targetSpeed;

                AcquireLock(tsLock);
                updateTrainSpeed(data, trainIndex, 0);
                delay = data->tstable[trainIndex]->timeRequiredToAchieveSpeed;
                ReleaseLock(tsLock);

                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "0 ");
                Delay(delay);
                trainset_reverse(message.num);

                if (data->numSensorPast > 0) {
                    data->expectNextSensorNum =  (data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE]->reverse)->num;
                    data->expectNextTimetick = 0;
                }

                AcquireLock(tsLock);
                data->tstable[trainIndex]->reverse = 1 - data->tstable[trainIndex]->reverse;
                updateTrainSpeed(data, trainIndex, speed);
                ReleaseLock(tsLock);

                trainset_setSpeed(message.num, speed);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "%d ", speed);
                break;
            case TRAINCTRL_TR_STOPAT:
                node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
                result = findRoute(data, trainIndex,
                                    node, data->tstable[trainIndex]->distance,
                                    message.location, message.data);
                if (result >= 0) {
                    data->tstable[trainIndex]->delayToStop = calculate_delayToStop(data, trainIndex, node, result);
                    data->tstable[trainIndex]->needToStop = 1;
                }
                break;
            case TRAINCTRL_HALT:
                AcquireLock(tsLock);
                updateTrainSpeed(data, trainIndex, 0);
                delay = data->tstable[trainIndex]->timeRequiredToAchieveSpeed;
                ReleaseLock(tsLock);

                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "0 ");
                Delay(message.delay);

                message.type = TRAINCTRL_HALT_COMPLETE;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
                Exit();
                break;
            default :
                warning("Send Train Control Message To Wrong Task (Train Task).");
        }
    }
}

void updateSpeedTable() {
    int serverTid;
    int msg = 0;
    TrainSetData *data;
    track_node *node, *nextSensor;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    int lastTime = Time();
    int currentTime;
    int timeadjust;
    int i = 0;
    for (;;i++) {
        timeadjust = 1;
        if (i % 100 == 0) {
            /* Adjust time difference periodically. Because we are assuming delay(1) = exactly 10ms. */
            currentTime = Time();
            if (currentTime != lastTime) {
                timeadjust += currentTime - lastTime;
                lastTime = currentTime;
            }
        }

        for(i = 0; i < TRAIN_NUM; i++) {
            AcquireLock(data->tstableLock[i]);
            data->tstable[i]->timetick = data->tstable[i]->timetick + 1;
            if (data->tstable[i]->needToStop) {
                data->tstable[i]->delayToStop = data->tstable[i]->delayToStop - timeadjust;
            }
            // TODO: give one sensor table to each train and update this code.
            if (data->numSensorPast > 0) {
                node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
                nextSensor = nextSensorOrExit(data, node);
                data->tstable[0]->distance = calculate_expectTravelledDistance(data, 0, nextSensor->friction);
            }
            ReleaseLock(data->tstableLock[i]);
        }

        Delay(1);
        lastTime++;
    }
}

void displayCurrentPosition() {
    TrainSetData *data;
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    track_node *node;
    for (;;) {
        if (data->numSensorPast > 0) {
            node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
            int distance = data->tstable[0]->distance;
            int maxDistance = nextSensorDistance(data, node);
            if ((data->tstable[0]->reverse) && (distance > (maxDistance + 20))) {
                distance = maxDistance + 20;
            }
            else if (!(data->tstable[0]->reverse) && (distance > (maxDistance + 140))) {
                distance = maxDistance + 140;
            }

            /* Change to more precious landmark. */
            for(;;) {
                int passed = nextDistance(data, node);
                if (node->type == NODE_EXIT) {
                    distance = 0;
                    break;
                }
                if (passed > distance) {
                    break;
                }
                distance = distance - passed;
                node = nextNode(data, node);
            }
            PrintfAt(COM2, TR_R, TRLOCATION_SENSOR_C, "%s   ", node->name);
            PrintfAt(COM2, TR_R, TRLOCATION_OFFSET_C, "%d ", distance / 10);
        }
        Delay(TIME_INTERVAL);
    }
}

void trainControlServer() {
    /* Trainset data initialization. */
    TrainSetData trainsetData;
    TrainSpeedData trainSpeedData[TRAIN_NUM];
    Lock tstableLock[TRAIN_NUM];
    int i = 0;
    for(i = 0; i < TRAIN_NUM; i++) {
        trainsetData.tstable[i] = &trainSpeedData[i];
        trainsetData.tstableLock[i] = &tstableLock[i];
    }
    track_node track[TRACK_MAX];
    trainsetData.track = track;
    Lock swtableLock;
    trainsetData.swtableLock = &swtableLock;

    TrainSetData *data = &trainsetData;

    trainset_init(data);
    IOidle(COM1);       // wait until initialization is done, i.e. IO idle.

    initializeUI(&trainsetData);            // draw initial UI frame.
    IOidle(COM2);       // wait until initialization is done, i.e. IO idle.

    switch (TRACK_USED) {
        case 'A':
            init_tracka(trainsetData.track);
            trackGraph_initTrackA(&trainsetData, trainsetData.track);
            break;
        case 'B':
            init_trackb(trainsetData.track);
            trackGraph_initTrackB(&trainsetData, trainsetData.track);
            break;
        default:
            warning("train(user task): Invalid TRACK_USED macro");
    }

    /* TrainIndex array */
    int trainNums[TRAIN_NUM] = {45, 49, 50};
    int trainTids[TRAIN_NUM] = {-1, -1, -1};

    /* Create children tasks. */
    int parentTid, childTid;
    int msg = 0;
    int trainTid = -1;

    Receive(&parentTid, &msg, sizeof(msg));

    childTid = Create(6, &sensorTask);      // Task to pull sensor feed.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(7, &updateSpeedTable);    // Task to update speed table.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(3, &displayCurrentPosition);    // Task to update speed table.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    int switchTid = Create(6, &switchTask);
    Send(switchTid, &data, sizeof(data), &msg, sizeof(msg));

    for (i = 0; i < TRAIN_NUM; i++) {
        trainTid = Create(6, &trainTask);
        trainTids[i] = trainTid;
        Send(trainTid, &data, sizeof(data), &msg, sizeof(msg));
        // temporarily set to 0 for all trains
        Send(trainTid, &msg, sizeof(msg), &msg, sizeof(msg));
    }

    RegisterAs("Train Control Server");

    /* Create the couriers. */
    CouriersStatus couriersStatus;
    for(i = 0; i < COURIER_NUM_MAX; i++) {
        Create(5, &courier);
    }
    couriersStatus.courierStartIndex = 0;
    couriersStatus.courierAvailable = 0;

    Reply(parentTid, &msg, sizeof(msg));

    int requesterTid;
    int haltingCount = 0;
    int haltingTid = -1;

    TrainControlMessage message;
    for(;;) {
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));

        trainTid = -1;
        for (i = 0; i < TRAIN_NUM; i++) {
            if (trainNums[i] == message.num) {
                trainTid = trainTids[i];
                break;
            }
        }

        switch (message.type) {
            case TRAINCTRL_INIT:
                Reply(requesterTid, &msg, sizeof(msg));
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_SETSPEED:
                Reply(requesterTid, &msg, sizeof(msg));
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_REVERSE:
                Reply(requesterTid, &msg, sizeof(msg));
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_STOPAT:
                msg = 1;
                Reply(requesterTid, &msg, sizeof(msg));
                if (data->numSensorPast > 0) {
                    sentTo(trainTid, &message, &couriersStatus);
                }
                break;
            case TRAINCTRL_SW_CHANGE:
                Reply(requesterTid, &msg, sizeof(msg));
                sentTo(switchTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_HALT:
                haltingTid = requesterTid;
                message.type = TRAINCTRL_HALT;
                for(i = 0; i < TRAIN_NUM; i++) {
                    sentTo(trainTids[i], &message, &couriersStatus);
                }
                break;
            case TRAINCTRL_HALT_COMPLETE:
                Reply(requesterTid, &msg, sizeof(msg));

                haltingCount = haltingCount + 1;
                if (haltingCount == TRAIN_NUM) {
                    trainset_stop();

                    /* Print out friction factor */
                    // for(i = 0 ; i < TRACK_MAX ; i++) {
                    //     node = (track_node *)(trainsetData.track + i);
                    //     if ((int)(1000 * node->friction) != 1000) {
                    //         Printf(COM2, "%d:%d ", i, (int)(1000 * node->friction));
                    //         if (i % 10 == 0) {
                    //             Printf(COM2, "\n");
                    //         }
                    //     }
                    // }

                    Reply(haltingTid, &msg, sizeof(msg));
                }
                break;
            case TRAINCTRL_COURIER_FREE:
                couriersStatus.courierTable[(couriersStatus.courierStartIndex + couriersStatus.courierAvailable) % COURIER_NUM_MAX] = requesterTid;
                couriersStatus.courierAvailable = couriersStatus.courierAvailable + 1;
                assert((couriersStatus.courierAvailable <= COURIER_NUM_MAX), "Courier Available Exceed Courier_Num_Max");
                break;
            default :
                warning("Unknown Train Control Message Type.");
        } // switch

        for(i = 0; i < TRAIN_NUM; i++) {
            AcquireLock(data->tstableLock[i]);
            if ((data->tstable[i]->needToStop) && (data->tstable[i]->delayToStop <= 0)) {
                data->tstable[i]->needToStop = 0;
                message.type = TRAINCTRL_TR_SETSPEED;
                message.num = data->tstable[i]->trainNum;
                message.data = 0;
                sentTo(trainTids[i], &message, &couriersStatus);
            }
            ReleaseLock(data->tstableLock[i]);
        }
    }
}

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

void sentTo(int destTid, TrainControlMessage *message, CouriersStatus *couriersStatus) {
    if (couriersStatus->courierAvailable > 0) {
        message->destTid = destTid;
        int courierTid = couriersStatus->courierTable[couriersStatus->courierStartIndex];
        couriersStatus->courierStartIndex = (couriersStatus->courierStartIndex + 1) % COURIER_NUM_MAX;
        couriersStatus->courierAvailable = couriersStatus->courierAvailable - 1;
        Reply(courierTid, message, sizeof(TrainControlMessage));
    }
    else {
        warning("Run out of courier");
    }
}

void updateTrainSpeed(TrainSetData *data, int trainIndex, int newSpeed) {
    data->tstable[trainIndex]->lastSpeed = data->tstable[trainIndex]->targetSpeed;
    data->tstable[trainIndex]->targetSpeed = newSpeed;
    data->tstable[trainIndex]->lastSpeedDuration = data->tstable[trainIndex]->timetick - data->tstable[trainIndex]->timetickWhenHittingSensor;
    assert((data->tstable[trainIndex]->lastSpeedDuration >= 0), "LastSpeed duration is negative");
    data->tstable[trainIndex]->timetick = 0;
    data->tstable[trainIndex]->timetickWhenHittingSensor = 0;
    data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);
}

int findRoute(struct TrainSetData *data, int trainIndex, track_node *start, int startOffset, char *location, int endOffset) {
    int i = 0;
    track_node *track = data->track;
    track_node *end = 0;

    for(i = 0; i < TRACK_MAX; i++) {
        track[i].visited = 0;
        if (stringEquals(track[i].name, location)) {
            end = &track[i];
        }
    }

    if (end == 0) {
        return -1;
    }

    data->tstable[trainIndex]->stopLocation = end;

    int result[TRACK_SWITCH_NUM];
    int distance = findRouteDistance(start, end, result, 0);
    if (distance < 0) {
        return -1;
    }
    distance -= startOffset;
    distance += endOffset;

    track_node *current = start;
    int switchNum, switchDir;
    for(i = 0;;) {
        if (current == end) {
            break;
        }
        if (current->type == NODE_BRANCH) {
            switchNum = current->num;
            switchDir = result[i];
            if (switchDir != *(data->swtable + getSwitchIndex(switchNum))) {
                if ((current == start) || (current == nextNode(data, start))) {
                    return -3;      // Too close to a switch
                }
                else {
                    /* Update switch table. */
                    AcquireLock(data->swtableLock);
                    *(data->swtable + getSwitchIndex(switchNum)) = switchDir;
                    ReleaseLock(data->swtableLock);
                    /* Send message to train set to actually turn switch. */
                    trainset_turnSwitch(switchNum, switchDir);
                    /* Update terminal switch table. */
                    updateSwitchTable(switchNum, switchDir);
                    /* Update realtime graph. */
                    trackGraph_turnSw(data, switchNum, switchDir);
                }
            }
            current = current->edge[switchDir].dest;
            i++;
        }
        else {
            current = current->edge[DIR_AHEAD].dest;
        }
    }
    return distance;
}
