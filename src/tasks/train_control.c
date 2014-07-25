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
void reverseTrainSpeed(TrainSetData *data, int trainIndex);
void updateAndPrintSensorEstimation(TrainSetData *data, int trainIndex);
void findAndStoreFinalLocation(struct TrainSetData *data, int trainIndex, char *location, int finalLocationOffset);
int findRoute(struct TrainSetData *data, int trainIndex);

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

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    int i;
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

            /* Highlight graph and update nextSensor in each train's trainData */
            for(i = 0; i < TRAIN_NUM; i++) {
                trdata = data->trtable[i];
                if (trdata->numSensorPast > 0) {
                    updateAndPrintSensorEstimation(data, i);
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

    /* Clean junk data */
    trainset_subscribeSensorFeeds();
    trainset_pullSensorFeeds(data);

    int someInit = 0;
    int i,j;
    for (;;) {
        for (j = 0; j < TRAIN_NUM; j++) {
            if (data->trtable[j]->init >= 0) {
                someInit = 1;
                break;
            }
        }
        if (someInit) {
            break;
        }
        Delay(TIME_INTERVAL);
    }
    for (;;) {
        Delay(TIME_INTERVAL);
        trainset_subscribeSensorFeeds();
        if (!trainset_pullSensorFeeds(data)) {
            continue;
        }
        for (i = 0; i < TRAIN_NUM; i++) {
            trdata = data->trtable[i];

            if (trdata->numSensorPast == 0) {
                continue;
            }

            if (trdata->lastSensor != data->sentable[(data->totalSensorPast - 1) % SENTABLE_SIZE]) {
                continue;
            }

            if (trdata->init > 0) {
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

            /* Last sensor */
            PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C, "%s ", trdata->lastSensor->name);
            /* Actual timetick for last sensor */
            displayTime((trdata->actualTimetickHittingLastSensor)/10, SENLAST_R + i * 3, SENLAST_C + 16);
            /* Estimate timeick and difference for last sensor */
            if (trdata->estimateTimetickHittingLastSensor < 0) {
                /* Estimate timetick for last sensor */
                PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C + 34, "INFI   ");
                /* Difference between actual and estimate */
                PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C + 48, "INFI");
            }
            else {
                /* Estimate timetick for last sensor */
                displayTime((trdata->estimateTimetickHittingLastSensor)/10, SENLAST_R + i * 3, SENLAST_C + 34);
                /* Difference between actual and estimate */
                int diff = (trdata->estimateTimetickHittingLastSensor - trdata->actualTimetickHittingLastSensor);
                if (diff < 0) {
                    diff = 0 - diff;
                }
                PrintfAt(COM2, SENLAST_R + i * 3, SENLAST_C + 48, "%d   ", diff > 999 ? 999 : diff);
            }
        }
    }
}

void trainTask() {
    int serverTid, courierTid;
    int trainIndex;
    int msg = 0;
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
            case TRAINCTRL_TR_INIT:
                trdata->reverse = 0;
                trdata->trainNum = message.num;
                trdata->nextSensor = (track_node *)0;
                if (stringEquals(message.location, "A10")) {
                    trdata->lastLandmark = &(data->track[9]);
                }
                else if (stringEquals(message.location, "A8")) {
                    trdata->lastLandmark = &(data->track[7]);
                }
                else if (stringEquals(message.location, "A5")) {
                    trdata->lastLandmark = &(data->track[4]);
                }
                trdata->init = 0;

                trdata->distanceAfterLastLandmark = 140;
                PrintfAt(COM2, TR_R + trainIndex * 3, TR_C, "Train%d  Speed :     Location :         +     cm", message.num);
                PrintfAt(COM2, SENEXPECT_R + trainIndex * 3, SWTABLE_C, "Expecting                at");
                PrintfAt(COM2,SENLAST_R + trainIndex * 3, SWTABLE_C, "Last Sensor         past at          expected          diff");

                /* Initialization movement. */
                trainset_setSpeed(message.num, 2);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "2 ");
                Delay(100);
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ");
                Delay(100);

                if (trdata->numSensorPast > 0) {
                    node = trdata->lastSensor;
                    if (nextSensorOrExit(data, node)->type == NODE_EXIT) {
                        trainset_setSpeed(message.num, 15);
                    }
                }
                break;
            case TRAINCTRL_TR_SETSPEED:
                updateTrainSpeed(data, trainIndex, message.data);
                trainset_setSpeed(message.num, message.data);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "%d ", message.data);
                break;
            case TRAINCTRL_TR_STOP:
                updateTrainSpeed(data, trainIndex, 0);
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ");
                Delay(trdata->delayRequiredToAchieveSpeed);
                if (trdata->continueToStop) {
                    // if ((trdata->nextLocation != nextNode(data, trdata->lastLandmark)) && (trdata->lastLandmark != nextNode(data, trdata->nextLocation))) {
                    //     AcquireLock(trLock);
                    //     trdata->continueToStop = 0;
                    //     trdata->needToStop = 0;
                    //     trdata->init = -1;
                    //     ReleaseLock(trLock);
                    //     PrintfAt(COM2, LOG_R, LOG_C, "%sTrain %d is lost. .%s",TCS_RED, trdata->trainNum, TCS_RESET, TCS_DELETE_TO_EOL);
                    //     Log("Train %d is lost.", trdata->trainNum);
                    // }
                    if ((trdata->nextLocation == trdata->finalLocation) || (trdata->nextLocation == trdata->finalLocationAlt)) {
                        AcquireLock(trLock);
                        trdata->continueToStop = 0;
                        trdata->nextLocation = (track_node *)0;
                        ReleaseLock(trLock);
                    }
                    else {
                        result = findRoute(data, trainIndex);
                        if (result >= 0) {
                            updateTrainSpeed(data, trainIndex, 8);

                            AcquireLock(trLock);
                            trdata->needToStop = 1;
                            trdata->continueToStop = 1;
                            trdata->delayToStop = calculate_delayToStop(data, trdata, trdata->nextLocation, result);
                            ReleaseLock(trLock);

                            trainset_setSpeed(message.num, 8);
                            PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "8 ");
                            Log("Delay %d ticks", trdata->delayToStop);
                        }
                    }
                }
                break;
            case TRAINCTRL_TR_REVERSE:
                reverseTrainSpeed(data, trainIndex);
                break;
            case TRAINCTRL_TR_GO:
                updateTrainSpeed(data, trainIndex, 0);
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ");
                Delay(trdata->delayRequiredToAchieveSpeed);
                findAndStoreFinalLocation(data, trainIndex, message.location, message.data);
                result = findRoute(data, trainIndex);
                if (result >= 0) {
                    updateTrainSpeed(data, trainIndex, 8);

                    AcquireLock(trLock);
                    trdata->needToStop = 1;
                    trdata->continueToStop = 1;
                    trdata->delayToStop = calculate_delayToStop(data, trdata, trdata->lastLandmark, result);
                    ReleaseLock(trLock);

                    trainset_setSpeed(message.num, 8);
                    PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "8 ");
                    Log("Delay %d ticks", trdata->delayToStop);
                }
                break;
            case TRAINCTRL_HALT:
                updateTrainSpeed(data, trainIndex, 0);

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
                Log("Lose count : %d", currentTime - lastTime);
                timeadjust += currentTime - lastTime;
                lastTime = currentTime;
            }
        }

        for(i = 0; i < TRAIN_NUM; i++) {
            trdata = data->trtable[i];
            sendToServer = 0;

            if (trdata->init < 0) {
                continue;
            }

            AcquireLock(data->trtableLock[i]);
            trdata->timetickSinceSpeedChange = trdata->timetickSinceSpeedChange + 1;
            ReleaseLock(data->trtableLock[i]);

            if (trdata->init == 0) {
                continue;
            }

            AcquireLock(data->trtableLock[i]);

            if (trdata->needToStop) {
                trdata->delayToStop = trdata->delayToStop - 1;
            }

            if ((trdata->needToStop) && (trdata->delayToStop <= 0)) {
                trdata->needToStop = 0;
                sendToServer = 1;
            }

            if ((trdata->stopInProgress) && (trdata->timetickSinceSpeedChange > trdata->delayRequiredToAchieveSpeed)) {
                trdata->lastLandmark = trdata->targetStopLandmark;
                trdata->distanceAfterLastLandmark = trdata->distanceAfterTargetStopLandmark;
            }

            double distanceAfterLastLandmark = trdata->distanceAfterLastLandmark;
            track_node *lastLandmark = trdata->lastLandmark;

            distanceAfterLastLandmark += calculate_currentVelocity(trdata, trdata->timetickSinceSpeedChange) * (trdata->nextSensor->friction);
            for(;;) {
                if (lastLandmark->type == NODE_EXIT) {
                    distanceAfterLastLandmark = 0;
                    break;
                }
                if (lastLandmark == trdata->nextSensor) {
                    if ((trdata->reverse) && (distanceAfterLastLandmark > 20)) {
                        distanceAfterLastLandmark = 20;
                    }
                    else if ((!trdata->reverse) && (distanceAfterLastLandmark > 140)) {
                        distanceAfterLastLandmark = 140;
                    }
                    break;
                }
                int distance = nextDistance(data, lastLandmark);
                if (distanceAfterLastLandmark < distance) {
                    break;
                }
                distanceAfterLastLandmark -= distance;
                lastLandmark = nextNode(data, lastLandmark);
            }

            trdata->distanceAfterLastLandmark = distanceAfterLastLandmark;
            trdata->lastLandmark = lastLandmark;

            if ((trdata->stopInProgress) && (trdata->timetickSinceSpeedChange > trdata->delayRequiredToAchieveSpeed)) {
                trdata->stopInProgress = 0;
                if (trdata->lastLandmark == trdata->lastSensor) {
                    if ((trdata->reverse) && (distanceAfterLastLandmark < 20)) {
                        distanceAfterLastLandmark = 20;
                    }
                    else if ((!trdata->reverse) && (distanceAfterLastLandmark < 140)) {
                        distanceAfterLastLandmark = 140;
                    }
                }
            }

            ReleaseLock(data->trtableLock[i]);

            // if (k % 10 == 0) {
            //     if ((trdata->expectTimetickHittingNextSensor > 0) &&
            //         ((trdata->expectTimetickHittingNextSensor + TIMEOUT) < lastTime)) {
            //         AcquireLock(data->trtableLock[i]);
            //         trdata->continueToStop = 0;
            //         trdata->needToStop = 0;
            //         trdata->init = -1;
            //         sendToServer = 1;
            //         ReleaseLock(data->trtableLock[i]);
            //         PrintfAt(COM2, LOG_R, LOG_C, "%sTrain %d is stalled or lost. .%s",TCS_RED, trdata->trainNum, TCS_RESET, TCS_DELETE_TO_EOL);
            //         Log("Train %d is stalled or lost.", trdata->trainNum);
            //     }
            // }

            if(sendToServer) {
                message.type = TRAINCTRL_TR_STOP;
                message.num = trdata->trainNum;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
            }
        }

        Delay(1);
        lastTime++;
    }
}

void trainCoordinator() {
    int serverTid;
    int msg = 0, i;
    int reservInited = 0;
    TrainSetData *data;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    for (;;) {
        for (i = 0; i< TRAIN_NUM; i++) {
            if ((data->trtable[i]->init == 0) && !(reservInited & (1 << i))) {
                reserv_init(data, i);
                reservInited = reservInited | (1 << i);
            } else if (data->trtable[i]->init > 0) {
                reserv_updateReservation(serverTid, data, i);
            }
        }

        Delay(RESERV_DELAY);
    }

}

void displayCurrentPosition() {
    int serverTid;
    int msg = 0;
    TrainSetData *data;
    TrainData *trdata;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    track_node *lastLandmark;
    double distanceAfterLastLandmark;
    int i = 0;
    for (;;) {
        for (i = 0; i < TRAIN_NUM; i++) {
            trdata = data->trtable[i];

            if (trdata->init > 0) {
                AcquireLock(data->trtableLock[i]);
                lastLandmark = trdata->lastLandmark;
                distanceAfterLastLandmark = trdata->distanceAfterLastLandmark;
                ReleaseLock(data->trtableLock[i]);

                PrintfAt(COM2, TR_R + i * 3, TRLOCATION_SENSOR_C, "%s   ", lastLandmark->name);
                PrintfAt(COM2, TR_R + i * 3, TRLOCATION_OFFSET_C, "%d ", (int)distanceAfterLastLandmark / 10);
            }
        }
        Delay(TIME_INTERVAL);
    }
}

void drawTrackGraph() {
    int serverTid;
    int msg = 0, i;
    TrainSetData *data;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    /* Initialize switch. */
    int swtableSnapShot[SWITCH_TOTAL];
    for (i = 0; i < SWITCH_TOTAL; i++) {
        swtableSnapShot[i] = data->swtable[i];
        trackGraph_drawSw(data, i, swtableSnapShot[i]);
    }

    /* Initialize track locations. */
    unsigned int trackGraphNodes[TRAIN_NUM][3];
    int redrawSwNextTimeForTrain[TRAIN_NUM];
    for (i = 0; i < TRAIN_NUM; i++) {
        trackGraphNodes[i][0] = -1;
        trackGraphNodes[i][1] = -1;
        trackGraphNodes[i][2] = -1;
        redrawSwNextTimeForTrain[i] = -1;
    }

    /* Each train a different color. */
    int numTrainColors = 1;
    int colors[numTrainColors];
    colors[0] = 31;     // red.

    /* Forever. */
    TrainData *trdata;
    int color;
    int loopCount;
    for (loopCount = 0; ; loopCount = (loopCount + 1) % 5) {
        /* Update switch table. */
        for (i = 0; i < SWITCH_TOTAL; i++) {
            trackGraph_redrawSw(data, i, swtableSnapShot[i], data->swtable[i]);
            swtableSnapShot[i] = data->swtable[i];
        }

        /* Update train. */
        for (i = 0; i < TRAIN_NUM; i++) {
            trdata = data->trtable[i];
            if (trdata->init < 0) {
                continue;
            }
            color = colors[i % numTrainColors];
            trackGraph_redrawTrainLoc(data, trackGraphNodes[i],
                trdata->lastLandmark, (unsigned int)trdata->distanceAfterLastLandmark,
                swtableSnapShot,
                &(redrawSwNextTimeForTrain[i]),
                color);

                // Debug printing
                if (loopCount == 0) {
                    // printMyReservationRange(data, i);
                }
        }
        Delay(20);
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
    Lock trackLock;
    trainsetData.trackLock = &trackLock;

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
    trainNums[2] = 53;
    int trainTids[TRAIN_NUM];

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

    childTid = Create(8, &updateTrainTable);                    // Task to update speed table.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(7, &trainCoordinator);                    // Reservation Task.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(3, &displayCurrentPosition);              // Task to display train location
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(3, &drawTrackGraph);                      // Task to draw train graph
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
            case TRAINCTRL_TR_INIT:
            case TRAINCTRL_TR_SETSPEED:
            case TRAINCTRL_TR_REVERSE:
            case TRAINCTRL_TR_STOP:
                Reply(requesterTid, &msg, sizeof(msg));
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_GO:
                Reply(requesterTid, &msg, sizeof(msg));
                if (data->trtable[trainIndex]->init > 0) {
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
                    if (data->trtable[i]->init > 0) {
                        message.num = data->trtable[i]->trainNum;
                        sentTo(trainTids[i], &message, &couriersStatus);
                    }
                    else {
                        haltingCount = haltingCount + 1;
                    }
                }
                if (haltingCount == TRAIN_NUM) {
                    trainset_stop();
                    Reply(haltingTid, &msg, sizeof(msg));
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

    AcquireLock(data->trtableLock[trainIndex]);

    trdata->lastSpeed = trdata->targetSpeed;
    trdata->targetSpeed = newSpeed;
    trdata->timetickSinceSpeedChange = 0;
    trdata->timetickWhenHittingSensor = 0;
    trdata->delayRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(trdata);

    if (newSpeed == 0) {
        trdata->stopInProgress = 1;
        if (trdata->shortMoveInProgress) {
            trdata->targetStopLandmark = trdata->nextLocation;
            trdata->distanceAfterTargetStopLandmark = trdata->nextLocationOffset;
        }
        else {
            trdata->targetStopLandmark = trdata->lastLandmark;
            trdata->distanceAfterTargetStopLandmark = trdata->distanceAfterLastLandmark + calculate_stopDistance(trdata->trainNum, trdata->lastSpeed) * trdata->nextSensor->friction;
        }
        trdata->shortMoveInProgress = 0;
    }

    ReleaseLock(data->trtableLock[trainIndex]);

    updateAndPrintSensorEstimation(data, trainIndex);
}

void reverseTrainSpeed(TrainSetData *data, int trainIndex) {
    TrainData *trdata = data->trtable[trainIndex];
    Lock *trLock = data->trtableLock[trainIndex];

    int speed = trdata->targetSpeed;
    int trainNum = trdata->trainNum;

    updateTrainSpeed(data, trainIndex, 0);
    trainset_setSpeed(trainNum, 0);
    PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "0 ");
    Delay(trdata->delayRequiredToAchieveSpeed);

    trainset_reverse(trainNum);

    AcquireLock(trLock);
    if (trdata->init > 0) {
        trdata->expectTimetickHittingNextSensor = -1;
        trdata->expectTimetickHittingNextNextSensor = -1;
        trdata->lastLandmark = trdata->lastLandmark->reverse;
        trdata->distanceAfterLastLandmark = TRAIN_LENGTH - trdata->distanceAfterLastLandmark;
        for(;;) {
            if (trdata->lastLandmark->type == NODE_EXIT) {
                trdata->distanceAfterLastLandmark = 0;
                break;
            }
            int distance = nextDistance(data, trdata->lastLandmark);
            if (trdata->distanceAfterLastLandmark < distance) {
                break;
            }
            trdata->distanceAfterLastLandmark -= distance;
            trdata->lastLandmark = nextNode(data, trdata->lastLandmark);
        }
    }
    trdata->reverseInProgress = 1;
    trdata->reverse = 1 - trdata->reverse;

    ReleaseLock(trLock);

    updateTrainSpeed(data, trainIndex, speed);
    trainset_setSpeed(trainNum, speed);
    PrintfAt(COM2, TR_R + trainIndex * 3, TRSPEED_C, "%d ", speed);
}

void updateAndPrintSensorEstimation(TrainSetData *data, int trainIndex) {
    TrainData *trdata = data->trtable[trainIndex];

    if (trdata->init <= 0) {
        return;
    }

    AcquireLock(data->trtableLock[trainIndex]);
    // track_node *oldSensor = trdata->nextSensor;
    trdata->nextSensor = nextSensorOrExit(data, trdata->lastLandmark);
    trdata->nextNextSensor = nextSensorOrExit(data, trdata->nextSensor);
    trdata->nextWrongSensor = nextWrongDirSensorOrExit(data, trdata->lastLandmark);
    trdata->nextSwitch = nextBranchOrExit(data, trdata->lastLandmark);

    /* Estimate timetick for next/nextNext sensor */
    int timeInterval = calculate_expectArrivalDuration(trdata, nextSensorDistance(data, trdata->lastLandmark) - (int)trdata->distanceAfterLastLandmark, trdata->nextSensor->friction);
    if (timeInterval < 0) {
        trdata->expectTimetickHittingNextSensor = -1;
    }
    else {
        trdata->expectTimetickHittingNextSensor = Time() + timeInterval;
    }
    timeInterval = calculate_expectArrivalDuration(trdata, nextSensorDistance(data, trdata->nextSensor) - (int)trdata->distanceAfterLastLandmark, trdata->nextNextSensor->friction);
    if ((trdata->expectTimetickHittingNextSensor < 0) || (timeInterval < 0)) {
        trdata->expectTimetickHittingNextNextSensor = -1;
    }
    else {
        trdata->expectTimetickHittingNextNextSensor = Time() + timeInterval;
    }

    ReleaseLock(data->trtableLock[trainIndex]);

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

void findAndStoreFinalLocation(struct TrainSetData *data, int trainIndex, char *location, int finalLocationOffset) {
    int i = 0;
    track_node *track = data->track;
    track_node *end = 0;
    track_node *end_alt = 0;

    for(i = 0; i < TRACK_MAX; i++) {
        if (stringEquals(track[i].name, location)) {
            end = &track[i];
        }
        if (stringEquals(track[i].reverse->name, location)) {
            end_alt = &track[i];
        }
    }

    assert((end != (track_node *)0), "Final Location not found.");
    assert((end_alt != (track_node *)0), "Final Location not found.");

    TrainData *trdata = data->trtable[trainIndex];
    AcquireLock(data->trtableLock[trainIndex]);
    trdata->finalLocation = end;
    trdata->finalLocationAlt = end_alt;
    trdata->finalLocationOffset = finalLocationOffset;
    ReleaseLock(data->trtableLock[trainIndex]);
}

int findRoute(struct TrainSetData *data, int trainIndex) {
    int i = 0;
    track_node *track = data->track;

    for(i = 0; i < TRACK_MAX; i++) {
        track[i].visited = 0;
    }

    TrainData *trdata = data->trtable[trainIndex];

    track_node *start;
    int startOffset;
    AcquireLock(data->trtableLock[trainIndex]);
    if (trdata->nextLocation == (track_node *)0) {
        start = trdata->lastLandmark;
        startOffset = (int)trdata->distanceAfterLastLandmark;
    }
    else {
        start = trdata->nextLocation;
        startOffset = trdata->nextLocationOffset;
    }
    ReleaseLock(data->trtableLock[trainIndex]);

    track_node *end = trdata->finalLocation;
    track_node *end_alt = trdata->finalLocationAlt;
    int finalLocationOffset = trdata->finalLocationOffset;

    int distance;
    int *result;
    int result1[TRACK_SWITCH_NUM * 2];
    int result2[TRACK_SWITCH_NUM * 2];

    Log("Finding route...");
    int distance1 = -1;
    int distance2 = -1;

    if (startOffset > 0) {
        /* Straight ahead */
        distance1 = findRouteDistance(nextNode(data, start), end, end_alt, finalLocationOffset, start, result1, 0) - finalLocationOffset;
        if (distance1 >= 0) {
            distance1 += nextDistance(data, start) - startOffset;
        }
    }
    else {
        /* Straight behind */
        distance1 = findRouteDistance(start, end, end_alt, finalLocationOffset, (track_node *)0, result1, 0) - finalLocationOffset;
        if (distance1 >= 0) {
            distance1 -= startOffset;
        }
    }

    if ((TRAIN_LENGTH - startOffset) > 0) {
        /* Reverse ahead */
        distance2 = findRouteDistance(nextNode(data, start->reverse), end, end_alt, finalLocationOffset, start->reverse, result2, 0) - finalLocationOffset;
        if (distance2 >= 0) {
            distance2 += nextDistance(data, start->reverse) - (TRAIN_LENGTH - startOffset);
        }
    }
    else {
        /* Reverse behind */
        distance2 = findRouteDistance(start->reverse, end, end_alt, finalLocationOffset, (track_node *)0, result2, 0) - finalLocationOffset;
        if (distance2 >= 0) {
            distance2 -= (TRAIN_LENGTH - startOffset);
        }
    }

    /* Pick shortest path and choose head behind current facing node */
    if ((distance1 >= 0) &&
        ((distance1 <= distance2) || (distance2 < 0))) {

        if (startOffset > 0) {
            startOffset = startOffset - nextDistance(data, start);
            start = nextNode(data, start);
        }

        result = result1;
        Log("route : %s - %d -> %s/%s", start->name, (0 - startOffset / 10), end->name, end_alt->name);
    }
    else if ((distance2 >= 0) &&
        ((distance2 <= distance1) || (distance1 < 0))) {

        reverseTrainSpeed(data, trainIndex);
        start = start->reverse;
        startOffset = TRAIN_LENGTH - startOffset;

        if (startOffset > 0) {
            startOffset = startOffset - nextDistance(data, start);
            start = nextNode(data, start);
        }

        result = result2;
        Log("route : %s - %d -> %s/%s", start->name, (0 - startOffset / 10), end->name, end_alt->name);
    }
    else {
        warning("No route found");
        return -1;
    }

    /* Go thru path, turn switch, recalculate distance, break at end point or reverse point */
    track_node *current = start;
    int nextLocationOffset = 0;
    distance = 0 - startOffset;
    int switchDir, switchIndex;
    unsigned int stopAtSwDirctions = 0;
    unsigned int stopAtSwInvolved = 0;

    for(i = 0;;) {
        if (current == end) {
            distance += finalLocationOffset;
            break;
        }
        else if (current == end_alt) {
            distance -= finalLocationOffset;
            break;
        }

        if (current->type == NODE_MERGE) {
            if (result[i] == DIR_REVERSE) {
                /* Add gap ahead of merge node */
                distance += REVERSE_GAP;
                nextLocationOffset = REVERSE_GAP;
                break;
            }
            distance += current->edge[DIR_AHEAD].dist;
            current = current->edge[DIR_AHEAD].dest;
            i++;
        }
        else if (current->type == NODE_BRANCH) {
            switchIndex = getSwitchIndex(current->num);
            switchDir = result[i];
            stopAtSwDirctions = stopAtSwDirctions | switchDir << switchIndex;
            stopAtSwInvolved = stopAtSwInvolved | 1 << switchIndex;
            distance += current->edge[switchDir].dist;
            current = current->edge[switchDir].dest;
            i++;
        }
        else {
            distance += current->edge[DIR_AHEAD].dist;
            current = current->edge[DIR_AHEAD].dest;
        }
    }

    /* Store this location and offset in trainData, which will be used as start point next round */
    AcquireLock(data->trtableLock[trainIndex]);
    data->trtable[trainIndex]->nextLocation = current;
    data->trtable[trainIndex]->nextLocationOffset = nextLocationOffset;
    trdata->stopAtSwDirctions = stopAtSwDirctions;
    trdata->stopAtSwInvolved = stopAtSwInvolved;
    ReleaseLock(data->trtableLock[trainIndex]);

    if (current == end) {
        Log("next location : %s + %d, distance = %d, final location : %s/%s", current->name, (finalLocationOffset + nextLocationOffset) / 10, distance, end->name, end_alt->name);
    }
    else if (current == end_alt) {
        Log("next location : %s - %d, distance = %d, final location : %s/%s", current->name, (finalLocationOffset - nextLocationOffset) / 10, distance, end->name, end_alt->name);
    }
    else {
        Log("next location : %s + %d, distance = %d, final location : %s/%s", current->name, (finalLocationOffset + nextLocationOffset) / 10, distance, end->name, end_alt->name);
    }

    /* Highlight graph */
    for(i = 0; i < TRAIN_NUM; i++) {
        trdata = data->trtable[i];
        if (trdata->numSensorPast > 0) {
            updateAndPrintSensorEstimation(data, i);
        }
    }

    return distance;
}
