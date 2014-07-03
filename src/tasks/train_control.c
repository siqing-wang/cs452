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

void switchTask() {
    int serverTid;
    int msg = 0;
    Receive(&serverTid, &msg, sizeof(msg));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    for(;;) {
        Receive(&serverTid, &message, sizeof(message));
        trainset_turnSwitch(message.num, message.data);
        updateSwitchTable(message.num, message.data);
        Reply(serverTid, &msg, sizeof(msg));
    }
}

void trainTask() {
    int serverTid;
    TrainSetData *data;
    int msg = 0;

    Receive(&serverTid, &data, sizeof(data));
    Reply(serverTid, &msg, sizeof(msg));

    TrainControlMessage message;
    for(;;) {
        Receive(&serverTid, &message, sizeof(message));
        switch (message.type) {
            case TRAINCTRL_TR_SETSPEED:
                trainset_setSpeed(message.num, message.data);
                break;
            case TRAINCTRL_TR_REVERSE:
                Delay(message.data);
                trainset_reverse(message.num);
                break;
            case TRAINCTRL_TR_STOPAT:
                break;
            case TRAINCTRL_HALT:
                Reply(serverTid, &msg, sizeof(msg));
                trainset_setSpeed(message.num, 0);
                Exit();
                break;
            default :
                warning("Send Train Control Message To Wrong Task.");
        }
        Reply(serverTid, &msg, sizeof(msg));
    }
}

void pullSensorFeed() {
    TrainSetData *data;
    int parentTid;
    int msg = 0;
    Receive(&parentTid, &data, sizeof(data));
    Reply(parentTid, &parentTid, sizeof(parentTid));

    TrainControlMessage message;
    message.type = TRAINCTRL_SEN_TRIGGERED;
    for (;;) {
        trainset_subscribeSensorFeeds();
        if (trainset_pullSensorFeeds(data)) {
            Send(parentTid, &message, sizeof(message), &msg, sizeof(msg));
        }
        Delay(1);
    }
}

void updateSpeedTable() {
    int parentTid;
    int msg = 0;
    Receive(&parentTid, &msg, sizeof(msg));
    Reply(parentTid, &msg, sizeof(msg));

    TrainControlMessage message;
    message.type = TRAINCTRL_UPDATE_TSTABLE;
    for (;;) {
        Delay(1);
        Send(parentTid, &message, sizeof(message), &msg, sizeof(msg));
    }
}

void undrawTrack() {
    TrainSetData *data;
    int parentTid;
    int msg = 0;
    Receive(&parentTid, &data, sizeof(data));
    Reply(parentTid, &msg, sizeof(msg));

    track_node *lastNode;

    for (;;) {
        Receive(&parentTid, &msg, sizeof(msg));

        if (data->numSensorPast > 1) {
            lastNode = data->sentable[(data->numSensorPast - 2) % SENTABLE_SIZE];
            trackGraph_unhighlightSenPath(data, lastNode);
        }

        Reply(parentTid, &msg, sizeof(msg));
    }
}

void drawTrack() {
    TrainSetData *data;
    int parentTid;
    int msg = 0;
    Receive(&parentTid, &data, sizeof(data));
    Reply(parentTid, &msg, sizeof(msg));

    track_node *node;
    TrainControlMessage message;
    for (;;) {
        Receive(&parentTid, &message, sizeof(message));

        /* Draw switch */
        if (message.num > 0) {
            trackGraph_turnSw(data, message.num, message.data2, message.data);
        }

        /* Draw route */
        node = data->sentable[(data->numSensorPast - 1) % SENTABLE_SIZE];
        trackGraph_highlightSenPath(data, node);

        Reply(parentTid, &msg, sizeof(msg));
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

    Receive(&parentTid, &msg, sizeof(msg));

    childTid = Create(2, &pullSensorFeed);      // Task to pull sensor feed.
    Send(childTid, &data, sizeof(data), &msg, sizeof(msg));

    childTid = Create(2, &updateSpeedTable);    // Task to update speed table.
    Send(childTid, &msg, sizeof(msg), &msg, sizeof(msg));

    int undrawTrackTid = Create(2, &undrawTrack);
    int drawTrackTid = Create(2, &drawTrack);
    Send(undrawTrackTid, &data, sizeof(data), &msg, sizeof(msg));
    Send(drawTrackTid, &data, sizeof(data), &msg, sizeof(msg));

    int switchTid = Create(4, &switchTask);
    Send(switchTid, &msg, sizeof(msg), &msg, sizeof(msg));

    int train49Tid = Create(4, &trainTask);
    Send(train49Tid, &data, sizeof(data), &msg, sizeof(msg));

    int train50Tid = Create(4, &trainTask);
    Send(train50Tid, &data, sizeof(data), &msg, sizeof(msg));

    Reply(parentTid, &msg, sizeof(msg));

    int requesterTid;
    int trainTid = -1;
    int trainIndex = -1;
    int maxDelay = 0;
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
                trainIndex = 1;
                break;
        }
        switch (message.type) {
            case TRAINCTRL_TR_SETSPEED:
                if (trainIndex >= 0) {
                    data->tstable[trainIndex]->lastSpeed = data->tstable[trainIndex]->targetSpeed;
                    data->tstable[trainIndex]->targetSpeed = message.data;
                    data->tstable[trainIndex]->timetick = 0;
                    data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);
                }
                Send(trainTid, &message, sizeof(message), &msg, sizeof(msg));
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case TRAINCTRL_TR_REVERSE:
                /* Slow down */
                message.type = TRAINCTRL_TR_SETSPEED;
                message.data = 0;
                if (trainIndex >= 0) {
                    data->tstable[trainIndex]->lastSpeed = data->tstable[trainIndex]->targetSpeed;
                    data->tstable[trainIndex]->targetSpeed = 0;
                    data->tstable[trainIndex]->timetick = 0;
                    data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);

                }
                Send(trainTid, &message, sizeof(message), &msg, sizeof(msg));

                /* Reverse */
                message.type = TRAINCTRL_TR_REVERSE;
                if (trainIndex >= 0) {
                    message.data = data->tstable[trainIndex]->timeRequiredToAchieveSpeed;
                }
                Send(trainTid, &message, sizeof(message), &msg, sizeof(msg));

                /* Speed up */
                message.type = TRAINCTRL_TR_SETSPEED;
                message.data = 0;
                if (trainIndex >= 0) {
                    data->tstable[trainIndex]->targetSpeed = data->tstable[trainIndex]->lastSpeed;
                    data->tstable[trainIndex]->lastSpeed = 0;
                    data->tstable[trainIndex]->timetick = 0;
                    data->tstable[trainIndex]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, trainIndex);
                    message.data = data->tstable[trainIndex]->targetSpeed;
                }
                Send(trainTid, &message, sizeof(message), &msg, sizeof(msg));
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case TRAINCTRL_TR_STOPAT:
                Send(trainTid, &message, sizeof(message), &msg, sizeof(msg));
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case TRAINCTRL_SW_CHANGE:
                Send(undrawTrackTid, &msg, sizeof(msg), &msg, sizeof(msg));
                message.data2 = *(data->swtable + getSwitchIndex(message.num));
                *(data->swtable + getSwitchIndex(message.num)) = message.data;
                Send(switchTid, &message, sizeof(message), &msg, sizeof(msg));
                Send(drawTrackTid, &message, sizeof(message), &msg, sizeof(msg));
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case TRAINCTRL_SEN_TRIGGERED:
                message.num = 0;
                Send(undrawTrackTid, &msg, sizeof(msg), &msg, sizeof(msg));
                Send(drawTrackTid, &message, sizeof(message), &msg, sizeof(msg));
                Reply(requesterTid, &msg, sizeof(msg));
                break;
            case TRAINCTRL_UPDATE_TSTABLE:
                Reply(requesterTid, &msg, sizeof(msg));
                for(i = 0; i < TRAIN_NUM; i++) {
                    data->tstable[i]->timetick = data->tstable[i]->timetick + 1;
                }
                break;
            case TRAINCTRL_HALT:
                message.type = TRAINCTRL_HALT;
                for(i = 0; i < TRAIN_NUM; i++) {
                    data->tstable[i]->lastSpeed = data->tstable[i]->targetSpeed;
                    data->tstable[i]->targetSpeed = 0;
                    data->tstable[i]->timetick = 0;
                    data->tstable[i]->timeRequiredToAchieveSpeed = calculate_delayToAchieveSpeed(data, i);
                    if (maxDelay < data->tstable[i]->timeRequiredToAchieveSpeed) {
                        maxDelay = data->tstable[i]->timeRequiredToAchieveSpeed;
                    }
                    message.num = data->tstable[i]->trainNum;
                    switch (message.num) {
                        case 49:
                            trainTid = train49Tid;
                            break;
                        case 50:
                            trainTid = train50Tid;
                            break;
                    }
                    Send(trainTid, &message, sizeof(message), &msg, sizeof(msg));
                }
                Delay(maxDelay);
                trainset_stop();
                Reply(requesterTid, &msg, sizeof(msg));
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
