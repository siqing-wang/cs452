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

void switchTask() {
    int serverTid, courierTid;
    int msg = 0;
    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    for(;;) {
        Receive(&courierTid, &message, sizeof(message));
        trainset_turnSwitch(message.num, message.data);
        updateSwitchTable(message.num, message.data);
        Reply(courierTid, &msg, sizeof(msg));
    }
}

void trainTask() {
    int serverTid, courierTid;
    int trainIndex;
    int msg = 0;

    Receive(&serverTid, &trainIndex, sizeof(trainIndex));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    for(;;) {
        Receive(&courierTid, &message, sizeof(message));
        switch (message.type) {
            case TRAINCTRL_INIT:
                trainset_setSpeed(message.num, 2);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "2 ", message.data);
                Delay(100);
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "0 ", message.data);
                Delay(100);

                message.type = TRAINCTRL_INIT_COMPLETE;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
                break;
            case TRAINCTRL_TR_SETSPEED:
                trainset_setSpeed(message.num, message.data);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "%d ", message.data);
                break;
            case TRAINCTRL_TR_REVERSE:
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "0 ");
                Delay(message.delay);

                message.type = TRAINCTRL_TR_REVERSE_COMPLETE;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
                break;
            case TRAINCTRL_TR_REVERSE_COMPLETE:
                trainset_reverse(message.num);
                trainset_setSpeed(message.num, message.data);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "%d ", message.data);
                break;
            case TRAINCTRL_TR_STOPAT:
                break;
            case TRAINCTRL_HALT:
                Reply(courierTid, &msg, sizeof(msg));
                trainset_setSpeed(message.num, 0);
                PrintfAt(COM2, TR_R + trainIndex, TRSPEED_C, "0 ");
                Delay(message.delay);

                message.type = TRAINCTRL_HALT_COMPLETE;
                Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
                Exit();
                break;
            default :
                warning("Send Train Control Message To Wrong Task.");
        }
        Reply(courierTid, &msg, sizeof(msg));
    }
}

void pullSensorFeed() {
    TrainSetData *data;
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    message.type = TRAINCTRL_SEN_TRIGGERED;
    for (;;) {
        trainset_subscribeSensorFeeds();
        if (trainset_pullSensorFeeds(data)) {
            Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
        }
        Delay(1);
    }
}

void updateSpeedTable() {
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    message.type = TRAINCTRL_UPDATE_TSTABLE;
    for (;;) {
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
        Delay(1);
    }
}

void displayCurrentPosition() {
    TrainSetData *data;
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    track_node *node, *nextSensor;
    for (;;) {
        if (data->numSensorPast > 0) {
            node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
            nextSensor = nextSensorOrExit(data, node);
            int distance = calculate_expectTravelledDistance(data, 0, nextSensor->friction);

            for(;;) {
                int passed = nextDistance(data, node);
                if (passed > distance) {
                    break;
                }
                if (node == nextSensor) {
                    distance = 0;
                    break;
                }
                distance = distance - passed;
                node = nextNode(data, node);
            }
            PrintfAt(COM2, TR_R, TRLOCATION_SENSOR_C, "%s  ", node->name);
            PrintfAt(COM2, TR_R, TRLOCATION_OFFSET_C, "%d ", distance / 10);
        }
        Delay(1);
    }
}

void undrawTrack() {
    TrainSetData *data;
    int serverTid, courierTid;
    int msg = 0;
    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    track_node *lastNode;
    TrainControlMessage message;
    for (;;) {
        Receive(&courierTid, &message, sizeof(message));

        if (message.num == 0) {
            /* Undraw called by sensor */
            if (data->numSensorPast > 1) {
                lastNode = data->sentable[(data->numSensorPast - 2) % SENTABLE_SIZE];
                trackGraph_unhighlightSenPath(data, lastNode);
            }
        }
        else {
            /* Undraw called by switch */
            if (data->numSensorPast > 0) {
                lastNode = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
                trackGraph_unhighlightSenPath(data, lastNode);
            }
        }


        message.type = TRAINCTRL_UNDRAW_COMPLETE;
        Send(serverTid, &message, sizeof(message), &msg, sizeof(msg));
        Reply(courierTid, &msg, sizeof(msg));
    }
}

void drawTrack() {
    TrainSetData *data;
    int serverTid, courierTid;
    int msg = 0;
    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    track_node *node;
    TrainControlMessage message;
    for (;;) {
        Receive(&courierTid, &message, sizeof(message));

        /* Draw switch */
        if (message.num > 0) {
            trackGraph_turnSw(data, message.num, message.data2, message.data);
        }

        /* Draw route */
        if (data->numSensorPast > 0) {
            node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
            trackGraph_highlightSenPath(data, node);
        }

        Reply(courierTid, &msg, sizeof(msg));
    }
}

void courier() {
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    for (;;) {
        /* Semd message to control server to announce it is freed and waiting for message */
        message.type = TRAINCTRL_COURIER_FREE;
        Send(serverTid, &message, sizeof(message), &message, sizeof(message));

        /* Send message to tasks */
        Send(message.destTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void trainControlServer() {
    /* Trainset data initialization. */
    TrainSetData trainsetData;
    TrainSpeedData trainSpeedData[TRAIN_NUM];
    int i = 0;
    for(i = 0; i < TRAIN_NUM; i++) {
        trainsetData.tstable[i] = &trainSpeedData[i];
    }
    track_node track[TRACK_MAX];
    trainsetData.track = track;

    TrainSetData *data = &trainsetData;

    trainset_init(data);
    IOidle(COM1);       // wait until initialization is done, i.e. IO idle.

    initializeUI(&trainsetData);
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

    int parentTid, childTid;
    int msg = 0;
    int trainIndex = -1;

    Receive(&parentTid, &msg, sizeof(msg));

    childTid = Create(3, &pullSensorFeed);      // Task to pull sensor feed.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(3, &updateSpeedTable);    // Task to update speed table.
    Send(childTid, &msg, sizeof(msg), &msg, sizeof(msg));

    int undrawTrackTid = Create(3, &undrawTrack);
    int drawTrackTid = Create(3, &drawTrack);
    Send(undrawTrackTid, &data, sizeof(data), &msg, sizeof(msg));
    Send(drawTrackTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(3, &displayCurrentPosition);    // Task to update speed table.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    for(i = 0; i < COURIER_NUM_MAX; i++) {
        childTid = Create(5, &courier);
        Send(childTid, &msg, sizeof(msg), &msg, sizeof(msg));
    }

    int switchTid = Create(6, &switchTask);
    Send(switchTid, &msg, sizeof(msg), &msg, sizeof(msg));

    int train49Tid = Create(6, &trainTask);
    trainIndex = 0;
    Send(train49Tid, &trainIndex, sizeof(trainIndex), &msg, sizeof(msg));

    int train50Tid = Create(6, &trainTask);
    trainIndex = 0;
    Send(train50Tid, &trainIndex, sizeof(trainIndex), &msg, sizeof(msg));

    Reply(parentTid, &msg, sizeof(msg));

    int requesterTid;
    int trainTid = -1;
    int haltingCount = 0;
    int haltingTid = -1;

    CouriersStatus couriersStatus;
    couriersStatus.courierStartIndex = 0;
    couriersStatus.courierAvailable = 0;

    track_node *node;
    TrainControlMessage message;
    for(;;) {
        /* Receive msg. */
        Receive(&requesterTid, &message, sizeof(message));
        switch (message.num) {
            case 49:
                trainTid = train49Tid;
                trainIndex = 0;
                break;
            case 50:
                trainTid = train50Tid;
                trainIndex = 0;
                break;
        }
        switch (message.type) {
            case TRAINCTRL_INIT:
                Reply(requesterTid, &msg, sizeof(msg));

                data->init = 0;
                data->tstable[0]->trainNum = message.num;
                PrintfAt(COM2, TR_R, TR_C, "Train%d  Speed : 2   Location :        +    cm", message.num);
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_INIT_COMPLETE:
                Reply(requesterTid, &msg, sizeof(msg));
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
                Reply(requesterTid, &msg, sizeof(msg));

                if (trainIndex >= 0) {
                    data->tstable[trainIndex]->lastSpeed = data->tstable[trainIndex]->targetSpeed;
                    data->tstable[trainIndex]->targetSpeed = message.data;
                    data->tstable[trainIndex]->lastSpeedDuration = data->tstable[trainIndex]->timetick - data->tstable[trainIndex]->timetickWhenHittingSensor;
                    assert((data->tstable[trainIndex]->lastSpeedDuration >= 0), "LastSpeed duration is negative");
                    data->tstable[trainIndex]->timetick = 0;
                    data->tstable[trainIndex]->timetickWhenHittingSensor = 0;
                    data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);
                }
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_REVERSE:
                Reply(requesterTid, &msg, sizeof(msg));

                message.type = TRAINCTRL_TR_REVERSE;
                message.delay = 0;
                if (trainIndex >= 0) {

                    data->tstable[trainIndex]->lastSpeed = data->tstable[trainIndex]->targetSpeed;
                    data->tstable[trainIndex]->targetSpeed = 0;
                    data->tstable[trainIndex]->lastSpeedDuration = data->tstable[trainIndex]->timetick - data->tstable[trainIndex]->timetickWhenHittingSensor;
                    assert((data->tstable[trainIndex]->lastSpeedDuration >= 0), "LastSpeed duration is negative");
                    data->tstable[trainIndex]->timetick = 0;
                    data->tstable[trainIndex]->timetickWhenHittingSensor = 0;
                    data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);
                    message.delay = data->tstable[trainIndex]->timeRequiredToAchieveSpeed;
                }
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_REVERSE_COMPLETE:
                Reply(requesterTid, &msg, sizeof(msg));

                data->tstable[trainIndex]->reverse = 1 - data->tstable[trainIndex]->reverse;
                data->tstable[trainIndex]->targetSpeed = data->tstable[trainIndex]->lastSpeed;
                data->tstable[trainIndex]->lastSpeed = 0;
                data->tstable[trainIndex]->lastSpeedDuration = data->tstable[trainIndex]->timetick - data->tstable[trainIndex]->timetickWhenHittingSensor;
                assert((data->tstable[trainIndex]->lastSpeedDuration >= 0), "LastSpeed duration is negative");
                data->tstable[trainIndex]->timetick = 0;
                data->tstable[trainIndex]->timetickWhenHittingSensor = 0;
                data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);

                message.data = data->tstable[trainIndex]->targetSpeed;
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_TR_STOPAT:
                Reply(requesterTid, &msg, sizeof(msg));
                sentTo(trainTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_SW_CHANGE:
                Reply(requesterTid, &msg, sizeof(msg));

                message.type = TRAINCTRL_UNDRAW;
                message.data2 = *(data->swtable + getSwitchIndex(message.num));
                sentTo(undrawTrackTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_SEN_TRIGGERED:
                Reply(requesterTid, &msg, sizeof(msg));

                message.type = TRAINCTRL_UNDRAW;
                message.num = 0;
                sentTo(undrawTrackTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_UNDRAW_COMPLETE:
                Reply(requesterTid, &msg, sizeof(msg));

                if (message.num > 0) {
                    *(data->swtable + getSwitchIndex(message.num)) = message.data;
                    sentTo(switchTid, &message, &couriersStatus);
                }

                message.type = TRAINCTRL_DRAW;
                sentTo(drawTrackTid, &message, &couriersStatus);
                break;
            case TRAINCTRL_UPDATE_TSTABLE:
                Reply(requesterTid, &msg, sizeof(msg));

                for(i = 0; i < TRAIN_NUM; i++) {
                    data->tstable[i]->timetick = data->tstable[i]->timetick + 1;
                }
                break;
            case TRAINCTRL_HALT:
                haltingTid = requesterTid;
                message.type = TRAINCTRL_HALT;
                for(i = 0; i < TRAIN_NUM; i++) {
                    data->tstable[i]->lastSpeed = data->tstable[i]->targetSpeed;
                    data->tstable[i]->targetSpeed = 0;
                    data->tstable[i]->lastSpeedDuration = data->tstable[i]->timetick - data->tstable[i]->timetickWhenHittingSensor;
                    assert((data->tstable[i]->lastSpeedDuration >= 0), "LastSpeed duration is negative");
                    data->tstable[i]->timetick = 0;
                    data->tstable[i]->timetickWhenHittingSensor = 0;
                    data->tstable[i]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, i);
                    message.num = data->tstable[i]->trainNum;
                    message.delay = data->tstable[i]->timeRequiredToAchieveSpeed;
                    switch (message.num) {
                        case 49:
                            trainTid = train49Tid;
                            break;
                        case 50:
                            trainTid = train50Tid;
                            break;
                    }
                    sentTo(trainTid, &message, &couriersStatus);
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
