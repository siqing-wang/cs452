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
    TrainData *trdata;
    Lock * trLock;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    int i;
    for(;;) {
        Receive(&courierTid, &message, sizeof(message));
        Reply(courierTid, &msg, sizeof(msg));

        assert(message.type == TRAINCTRL_SW_CHANGE, "switchTask: Invalid message type.");

        if (message.data != *(data->swtable + getSwitchIndex(message.num))) {
            /* Unhighlight graph */
            for (i = 0; i < TRAIN_NUM; i++) {
                trdata = data->trtable[i];
                if (trdata->numSensorPast > 0) {
                    trackGraph_unhighlightSenPath(data, trdata->lastSensor);
                }
            }

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

            /* Highlight graph and update nextSensor in each train's trainData */
            for(i = 0; i < TRAIN_NUM; i++) {
                trdata = data->trtable[i];
                trLock = data->trtableLock[i];

                if (trdata->numSensorPast == 0) {
                    continue;
                }

                trackGraph_highlightSenPath(data, trdata->lastSensor);

                AcquireLock(trLock);
                trdata->nextSensor = nextSensorOrExit(data, trdata->lastSensor);
                trdata->nextNextSensor = nextSensorOrExit(data, trdata->nextSensor);

                /* Estimate timetick for next/nextNext sensor */
                int timeInterval = expectSensorArrivalTimeDuration(data, i, trdata->lastSensor, trdata->nextSensor->friction);
                if (timeInterval < 0) {
                    trdata->expectTimetickHittingNextSensor = -1;
                }
                else {
                    trdata->expectTimetickHittingNextSensor = trdata->actualTimetickHittingLastSensor + timeInterval;
                }
                timeInterval = expectSensorArrivalTimeDuration(data, i, trdata->nextSensor, trdata->nextNextSensor->friction);
                if ((trdata->expectTimetickHittingNextSensor) || (timeInterval < 0)) {
                    trdata->expectTimetickHittingNextNextSensor = -1;
                }
                else {
                    trdata->expectTimetickHittingNextNextSensor = trdata->expectTimetickHittingNextSensor + timeInterval;
                }
                ReleaseLock(trLock);

                /* Next sensor */
                PrintfAt(COM2, SENEXPECT_R + i * 3, SENEXPECT_C, "%s ", trdata->nextSensor->name);
                /* Estimate timetick fot next sensor */
                if (trdata->expectTimetickHittingNextSensor < 0) {
                    PrintfAt(COM2, SENEXPECT_R + i * 3, SENEXPECT_C + 16, "INFI   ");
                }
                else {
                    displayTime((trdata->expectTimetickHittingNextSensor)/10, SENEXPECT_R + i * 3, SENEXPECT_C + 16);
                }
            }
        }

    }
}

void sensorTask() {
    int serverTid;
    int msg = 0;
    TrainSetData *data;
    TrainData *trdata;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    message.type = TRAINCTRL_SEN_TRIGGERED;
    int i;
    for (;;) {
        trainset_subscribeSensorFeeds();
        if (trainset_pullSensorFeeds(data)) {
            for (i = 0; i < TRAIN_NUM; i++) {
                trdata = data->trtable[i];

                if (trdata->numSensorPast == 0) {
                    continue;
                }

                /* Next sensor */
                PrintfAt(COM2, SENEXPECT_R + i * 3, SENEXPECT_C, "%s ", trdata->nextSensor->name);
                /* Estimate timetick fot next sensor */
                if (trdata->expectTimetickHittingNextSensor < 0) {
                    PrintfAt(COM2, SENEXPECT_R + i * 3, SENEXPECT_C + 16, "INFI   ");
                }
                else {
                    displayTime((trdata->expectTimetickHittingNextSensor)/10, SENEXPECT_R + i * 3, SENEXPECT_C + 16);
                }

                /* Last sensor */
                PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C, "%s ", trdata->lastSensor->name);
                /* Actual timetick for last sensor */
                displayTime((trdata->actualTimetickHittingLastSensor)/10, SENLAST_R + i * 3, SENLAST_C + 16);
                /* Estimate timeick and difference for last sensor */
                if (trdata->estimateTimetickHittingLastSensor < 0) {
                    /* Estimate timetick for last sensor */
                    PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C + 34, "INFI   ");
                    /* Difference between actual and estimate */
                    PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C + 48, "INFI%s", TCS_DELETE_TO_EOL);
                }
                else {
                    /* Estimate timetick for last sensor */
                    displayTime((trdata->estimateTimetickHittingLastSensor)/10, SENLAST_R + i * 3, SENLAST_C + 34);
                    /* Difference between actual and estimate */
                    int diff = (trdata->estimateTimetickHittingLastSensor - trdata->actualTimetickHittingLastSensor);
                    if (diff < 0) {
                        diff = 0 - diff;
                    }
                    PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C + 48, "%d%s", diff, TCS_DELETE_TO_EOL);
                }

                /* Update terminal track graph. */
                if (trdata->numSensorPast > 1) {
                    trackGraph_unhighlightSenPath(data, trdata->lastLastSensor);
                }
                trackGraph_highlightSenPath(data, trdata->lastSensor);
            }
        }
        Delay(TIME_INTERVAL);
    }
}

void trainTask() {
    int serverTid, courierTid;
    int trainIndex;
    int msg = 0;
    int speed = 0;
    int result = 0;
    TrainSetData *data;
    TrainData *trdata;
    track_node *node;
    Lock *trLock;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    Receive(&serverTid, &trainIndex, sizeof(trainIndex));
    Reply(serverTid, &msg, sizeof(msg));

    trdata = data->trtable[trainIndex];
    trLock = data->trtableLock[trainIndex];

    TrainControlMessage message;
    for(;;) {
        Receive(&courierTid, &message, sizeof(message));
        Reply(courierTid, &msg, sizeof(msg));

        switch (message.type) {
            case TRAINCTRL_INIT:
                trdata->init = 0;
                trdata->trainNum = message.num;
                PrintfAt(COM2, TR_R + trainIndex * 3, TR_C, "Train%d  Speed :     Location :         +     cm", message.num);

                /* Initialization movement. */
                trainset_setSpeed(message.num, 2);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "2 ", message.data);
                Delay(100);
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ", message.data);
                Delay(100);

                if (trdata->numSensorPast > 0) {
                    node = trdata->lastSensor;
                    if (nextSensorOrExit(data, node)->type == NODE_EXIT) {
                        trainset_setSpeed(message.num, 15);
                        AcquireLock(trLock);
                        trdata->nextSensor = node->reverse;
                        trdata->nextNextSensor =  nextSensorOrExit(data, node->reverse);
                        ReleaseLock(trLock);
                    }
                }
                trdata->init = 1;
                break;
            case TRAINCTRL_TR_SETSPEED:
                AcquireLock(trLock);
                updateTrainSpeed(data, trainIndex, message.data);
                ReleaseLock(trLock);
                trainset_setSpeed(message.num, message.data);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "%d ", message.data);
                break;
            case TRAINCTRL_TR_REVERSE:
                speed = trdata->targetSpeed;

                AcquireLock(trLock);
                updateTrainSpeed(data, trainIndex, 0);
                ReleaseLock(trLock);

                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ");
                Delay(trdata->delayRequiredToAchieveSpeed);
                trainset_reverse(message.num);

                if (trdata->numSensorPast > 0) {
                    AcquireLock(trLock);
                    node = trdata->lastSensor;
                    trdata->nextSensor =  trdata->lastSensor->reverse;
                    trdata->expectTimetickHittingNextSensor = trdata->actualTimetickHittingLastSensor +
                                                2 * trdata->lastSpeedDurationAfterHittingLastSensor +
                                                2 * trdata->delayRequiredToAchieveSpeed;
                    trdata->nextNextSensor =  nextSensorOrExit(data, trdata->nextSensor);
                    trdata->expectTimetickHittingNextNextSensor = trdata->expectTimetickHittingNextSensor;
                    ReleaseLock(trLock);
                }

                AcquireLock(trLock);
                trdata->reverse = 1 - trdata->reverse;
                updateTrainSpeed(data, trainIndex, speed);
                ReleaseLock(trLock);

                trainset_setSpeed(message.num, speed);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "%d ", speed);
                break;
            case TRAINCTRL_TR_STOPAT:
                node = trdata->lastSensor;
                result = findRoute(data, trainIndex,
                                    node, trdata->distanceAfterLastSensor,
                                    message.location, message.data);
                if (result >= 0) {
                    AcquireLock(trLock);
                    trdata->needToStop = 1;
                    trdata->delayToStop = calculate_delayToStop(data, trdata, node, result);
                    ReleaseLock(trLock);
                }
                break;
            case TRAINCTRL_HALT:
                AcquireLock(trLock);
                updateTrainSpeed(data, trainIndex, 0);
                ReleaseLock(trLock);

                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ");
                Delay(trdata->delayRequiredToAchieveSpeed);

                message.type = TRAINCTRL_HALT_COMPLETE;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
                Exit();
                break;
            default :
                warning("Send Train Control Message To Wrong Task (Train Task).");
        }
    }
}

void updateTrainTable() {
    int serverTid;
    int msg = 0;
    TrainSetData *data;
    TrainData *trdata;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    int lastTime = Time();
    int currentTime;
    int timeadjust;

    int sendToServer = 0;

    int i = 0;
    int k = 0;
    TrainControlMessage message;
    for (;;k++) {
        timeadjust = 1;
        if (k % 100 == 0) {
            /* Adjust time difference periodically. Because we are assuming delay(1) = exactly 10ms. */
            currentTime = Time();
            if (currentTime != lastTime) {
                timeadjust += currentTime - lastTime;
                lastTime = currentTime;
            }
        }

        for(i = 0; i < TRAIN_NUM; i++) {
            trdata = data->trtable[i];
            sendToServer = 0;

            AcquireLock(data->trtableLock[i]);
            trdata->timetickSinceSpeedChange = trdata->timetickSinceSpeedChange + 1;
            if (trdata->needToStop) {
                trdata->delayToStop = trdata->delayToStop - timeadjust;
            }

            // PrintfAt(COM2, 45, 1, "%d%s", data->trtable[0]->delayToStop, TCS_DELETE_TO_EOL);
            if ((trdata->needToStop) && (trdata->delayToStop <= 0)) {
                trdata->needToStop = 0;
                sendToServer = 1;
            }

            if (trdata->numSensorPast > 0) {
                trdata->distanceAfterLastSensor = calculate_expectTravelledDistance(trdata, trdata->nextSensor->friction);
            }
            ReleaseLock(data->trtableLock[i]);

            if(sendToServer) {
                message.type = TRAINCTRL_TR_SETSPEED;
                message.num = trdata->trainNum;
                message.data = 0;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
            }
        }

        Delay(1);
        lastTime++;
    }
}

void displayCurrentPosition() {
    int serverTid;
    int msg = 0;
    TrainSetData *data;
    TrainData *trdata;
    track_node *node;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    int i = 0;
    for (;;) {
        for (i = 0; i < TRAIN_NUM; i++) {
            trdata = data->trtable[i];

            if (trdata->numSensorPast > 0) {
                node = trdata->lastSensor;
                int distance = trdata->distanceAfterLastSensor;
                int maxDistance = nextSensorDistance(data, node);
                if ((trdata->reverse) && (distance > (maxDistance + 20))) {
                    distance = maxDistance + 20;
                }
                else if (!(trdata->reverse) && (distance > (maxDistance + 140))) {
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
                PrintfAt(COM2, TR_R + i * 3, TRLOCATION_SENSOR_C, "%s   ", node->name);
                PrintfAt(COM2, TR_R + i * 3, TRLOCATION_OFFSET_C, "%d ", distance / 10);
            }
        }
        Delay(TIME_INTERVAL);
    }
}

void trainControlServer() {
    /* Trainset data initialization. */
    TrainSetData trainsetData;
    TrainData trainSpeedData[TRAIN_NUM];
    Lock trtableLock[TRAIN_NUM];
    int i = 0;
    for(i = 0; i < TRAIN_NUM; i++) {
        trainsetData.trtable[i] = &trainSpeedData[i];
        trainsetData.trtableLock[i] = &trtableLock[i];
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
    int trainNums[TRAIN_NUM];
    trainNums[0] = 45;
    trainNums[1] = 49;
    int trainTids[TRAIN_NUM] = {-1, -1};

    /* Create children tasks. */
    int parentTid, childTid;
    int msg = 0;
    int trainTid = -1;
    int trainIndex = 0;

    Receive(&parentTid, &msg, sizeof(msg));

    int switchTid = Create(6, &switchTask);                     // Task to turn switch
    Send(switchTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(6, &sensorTask);                          // Task to pull sensor feed.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    for (i = 0; i < TRAIN_NUM; i++) {
        trainTid = Create(6, &trainTask);
        trainTids[i] = trainTid;
        Send(trainTid, &data, sizeof(data), &msg, sizeof(msg));
        Send(trainTid, &i, sizeof(i), &msg, sizeof(msg));
    }

    childTid = Create(7, &updateTrainTable);                    // Task to update speed table.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(3, &displayCurrentPosition);              // Task to display train location
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

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
                trainIndex = i;
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
                Reply(requesterTid, &msg, sizeof(msg));
                if (data->trtable[trainIndex]->numSensorPast > 0) {
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
                    message.num = data->trtable[i]->trainNum;
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
    TrainData *trdata = data->trtable[trainIndex];
    trdata->lastSpeed = trdata->targetSpeed;
    trdata->targetSpeed = newSpeed;
    trdata->lastSpeedDurationAfterHittingLastSensor = trdata->timetickSinceSpeedChange - trdata->timetickWhenHittingSensor;
    assert((trdata->lastSpeedDurationAfterHittingLastSensor >= 0), "updateTrainSpeed : LastSpeed duration is negative");
    trdata->timetickSinceSpeedChange = 0;
    trdata->timetickWhenHittingSensor = 0;
    trdata->delayRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(trdata);

    /* Estimate timetick for next/nextNext sensor */
    int timeInterval = expectSensorArrivalTimeDuration(data, trainIndex, trdata->lastSensor, trdata->nextSensor->friction);
    if (timeInterval < 0) {
        trdata->expectTimetickHittingNextSensor = -1;
    }
    else {
        trdata->expectTimetickHittingNextSensor = trdata->actualTimetickHittingLastSensor + timeInterval;
    }
    timeInterval = expectSensorArrivalTimeDuration(data, trainIndex, trdata->nextSensor, trdata->nextNextSensor->friction);
    if ((trdata->expectTimetickHittingNextSensor) || (timeInterval < 0)) {
        trdata->expectTimetickHittingNextNextSensor = -1;
    }
    else {
        trdata->expectTimetickHittingNextNextSensor = trdata->expectTimetickHittingNextSensor + timeInterval;
    }

    /* Next sensor */
    PrintfAt(COM2, SENEXPECT_R + trainIndex * 3, SENEXPECT_C, "%s ", trdata->nextSensor->name);
    /* Estimate timetick fot next sensor */
    if (trdata->expectTimetickHittingNextSensor < 0) {
        PrintfAt(COM2, SENEXPECT_R + trainIndex * 3, SENEXPECT_C + 16, "INFI   ");
    }
    else {
        displayTime((trdata->expectTimetickHittingNextSensor)/10, SENEXPECT_R + trainIndex * 3, SENEXPECT_C + 16);
    }

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

    TrainData *trdata = data->trtable[trainIndex];
    Lock *trLock = data->trtableLock[trainIndex];

    AcquireLock(trLock);
    trdata->stopLocation = end;
    ReleaseLock(trLock);

    int result[TRACK_SWITCH_NUM];
    int distance = findRouteDistance(start, end, result, 0);
    if (distance < 0) {
        warning("No route found");
        return -1;
    }
    distance -= startOffset;
    distance += endOffset;

    for(i = 0; i < TRAIN_NUM; i++) {
        trdata = data->trtable[i];
        if (trdata->numSensorPast > 0) {
            trackGraph_unhighlightSenPath(data, trdata->lastSensor);
        }
    }

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
                    warning("Too close to switch");
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

    for(i = 0; i < TRAIN_NUM; i++) {
        trdata = data->trtable[i];
        trLock = data->trtableLock[i];

        if (trdata->numSensorPast == 0) {
            continue;
        }

        trackGraph_highlightSenPath(data, trdata->lastSensor);

        AcquireLock(trLock);
        trdata->nextSensor = nextSensorOrExit(data, trdata->lastSensor);
        trdata->nextNextSensor = nextSensorOrExit(data, trdata->nextSensor);

        /* Estimate timetick for next/nextNext sensor */
        int timeInterval = expectSensorArrivalTimeDuration(data, i, trdata->lastSensor, trdata->nextSensor->friction);
        if (timeInterval < 0) {
            trdata->expectTimetickHittingNextSensor = -1;
        }
        else {
            trdata->expectTimetickHittingNextSensor = trdata->actualTimetickHittingLastSensor + timeInterval;
        }
        timeInterval = expectSensorArrivalTimeDuration(data, i, trdata->nextSensor, trdata->nextNextSensor->friction);
        if ((trdata->expectTimetickHittingNextSensor) || (timeInterval < 0)) {
            trdata->expectTimetickHittingNextNextSensor = -1;
        }
        else {
            trdata->expectTimetickHittingNextNextSensor = trdata->expectTimetickHittingNextSensor + timeInterval;
        }
        ReleaseLock(trLock);

        /* Next sensor */
        PrintfAt(COM2, SENEXPECT_R + i * 3, SENEXPECT_C, "%s ", trdata->nextSensor->name);
        /* Estimate timetick fot next sensor */
        if (trdata->expectTimetickHittingNextSensor < 0) {
            PrintfAt(COM2, SENEXPECT_R + i * 3, SENEXPECT_C + 16, "INFI   ");
        }
        else {
            displayTime((trdata->expectTimetickHittingNextSensor)/10, SENEXPECT_R + i * 3, SENEXPECT_C + 16);
        }
    }

    return distance;
}
