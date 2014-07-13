/* train_calibration.c - All data(speed/distance) related with train */

#include <train_calibration.h>

double calculate_trainVelocity(int trainNum, int speed) {
    if (speed == 0) {
        return 0;
    }

    double velocity = -1;
    if (speed == 1) {
        velocity = 0.140815433;
    }
    if ((speed >= 2) && (speed <= 12)) {
        velocity =  0.52468 * speed - 0.24942;
    }
    else if (speed == 13) {
        velocity = 6.091978731;
    }
    else if (speed == 14) {
        velocity = 5.960918512;
    }
    switch(trainNum) {
        case 45:
            velocity = velocity * 1;
            break;
        case 49:
            velocity = velocity * 1;
            break;
        case 50:
            velocity = velocity * 0.93;
            break;
    }
    return velocity;
}

double calculate_stopDistance(int trainNum, int speed) {
    if ((speed < 0) || (speed > 14)) {
        return -1;
    }
    if (speed == 0) {
        return 0;
    }

    switch(trainNum) {
        case 45:
        case 49:
            switch(speed) {
                case 1:
                    return 7.25;
                case 2:
                    return 64.125;
                case 3:
                    return 139.875;
                case 4:
                    return 208;
                case 5:
                    return 269.625;
                case 6:
                    return 335.625;
                case 7:
                    return 425.625;
                case 8:
                    return 470.125;
                case 9:
                    return 536.5;
                case 10:
                    return 597.5;
                case 11:
                    return 681.375;
                case 12:
                    return 746;
                case 13:
                    return 817.375;
                case 14:
                    return 801;
            }
        case 50:
            switch(speed) {
                case 1:
                    return 10;
                case 2:
                    return 57.125;
                case 3:
                    return 126.5;
                case 4:
                    return 178.25;
                case 5:
                    return 222.5;
                case 6:
                    return 305.75;
                case 7:
                    return 385.125;
                case 8:
                    return 441.3755;
                case 9:
                    return 488.625;
                case 10:
                    return 545.625;
                case 11:
                    return 613.25;
                case 12:
                    return 678;
                case 13:
                    return 734.625;
                case 14:
                    return 735.875;
            }
    }
    return -1;
}

double calculate_currentVelocity(TrainData *trainData, int timetick) {
    int trainNum = trainData->trainNum;
    int lastSpeed = trainData->lastSpeed;
    int targetSpeed = trainData->targetSpeed;

    double lastVelocity = calculate_trainVelocity(trainNum, lastSpeed);
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);

    if ((lastVelocity < 0) || (targetVelocity < 0)) {
        return -1;
    }

    if (timetick == 0) {
        return lastVelocity;
    }
    else if (timetick >= trainData->delayRequiredToAchieveSpeed) {
        return targetVelocity;
    }

    double distance = calculate_stopDistance(trainNum, targetSpeed) - calculate_stopDistance(trainNum, lastSpeed);
    if (distance < 0) {
        distance = 0 - distance;
    }

    double velocityDiff = targetVelocity - lastVelocity;
    double temp = (lastVelocity + targetVelocity) / (2 * distance) * timetick;

    double velocity = lastVelocity;
    velocity += 3 * velocityDiff * temp * temp;
    velocity -= 2 * velocityDiff * temp * temp * temp;
    return velocity;
}

int calculate_delayToAchieveSpeed(TrainData *trainData) {
    int trainNum = trainData->trainNum;
    int lastSpeed = trainData->lastSpeed;
    int targetSpeed = trainData->targetSpeed;

    double lastVelocity = calculate_trainVelocity(trainNum, lastSpeed);
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);

    double distance = calculate_stopDistance(trainNum, targetSpeed) - calculate_stopDistance(trainNum, lastSpeed);
    if (distance < 0) {
        distance = 0 - distance;
    }
    else if (distance == 0) {
        return 0;
    }

    return ((2 * distance) / (lastVelocity + targetVelocity));
}

int calculate_expectArrivalDuration(TrainData *trainData, int distance, double friction) {
    int trainNum = trainData->trainNum;
    int targetSpeed = trainData->targetSpeed;
    int timetickSinceSpeedChange = trainData->timetickSinceSpeedChange;
    int delayRequiredToAchieveSpeed = trainData->delayRequiredToAchieveSpeed;

    double currentVelocity = 0;
    double currentDistance = 0;
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);

    int tick = 0;
    for(;;) {
        if (currentDistance >= distance) {
            break;
        }
        if ((timetickSinceSpeedChange + tick) >= delayRequiredToAchieveSpeed) {
            break;
        }
        currentVelocity = calculate_currentVelocity(trainData, (timetickSinceSpeedChange+ tick)) * friction;
        currentDistance += currentVelocity;
        tick ++;
    }

    if (currentDistance < distance) {
        if (targetVelocity == 0) {
            return -1;
        }
        tick += ((distance - currentDistance) / (targetVelocity * friction));
    }

    return tick;
}

int calculate_expectTravelledDistance(TrainData *trainData, double friction) {
    int trainNum = trainData->trainNum;
    int lastSpeed = trainData->lastSpeed;
    int targetSpeed = trainData->targetSpeed;
    int timetickSinceSpeedChange = trainData->timetickSinceSpeedChange;
    int delayRequiredToAchieveSpeed = trainData->delayRequiredToAchieveSpeed;
    int timetickWhenHittingSensor = trainData->timetickWhenHittingSensor;
    int lastSpeedDurationAfterHittingLastSensor = trainData->lastSpeedDurationAfterHittingLastSensor;

    double lastVelocity = calculate_trainVelocity(trainNum, lastSpeed);
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);
    double currentVelocity = 0;
    double currentDistance = lastSpeedDurationAfterHittingLastSensor * (lastVelocity * friction);

    int tick = timetickWhenHittingSensor;
    for(;;) {
        if (tick >= delayRequiredToAchieveSpeed) {
            break;
        }
        if (tick >= timetickSinceSpeedChange) {
            break;
        }
        currentVelocity = calculate_currentVelocity(trainData, tick) * friction;
        currentDistance += currentVelocity;
        tick ++;
    }

    if (tick < timetickSinceSpeedChange) {
        currentDistance += (timetickSinceSpeedChange - tick) * (targetVelocity * friction);
    }

    if (trainData->reverse) {
        return (int)currentDistance + 20;
    }
    else {
        return (int)currentDistance + 140;
    }
}

int calculate_delayToStop(TrainSetData *trainSetData, TrainData *trainData, track_node *start, int distance) {
    int trainNum = trainData->trainNum;
    int speed = trainData->targetSpeed;
    int timetickSinceSpeedChange = trainData->timetickSinceSpeedChange;

    double minDistance = calculate_stopDistance(trainNum, speed);

    int delay = 0;
    if (distance >= (2 * minDistance)) {
        distance -= minDistance;
        for(;;) {
            if (start->type == NODE_EXIT) {
                break;
            }
            int passed = nextSensorDistance(trainSetData, start);
            if (distance < passed) {
                break;
            }
            distance = distance - passed;
            start = nextSensorOrExit(trainSetData, start);
            delay += calculate_expectArrivalDuration(trainData, passed, start->friction);
        }

        delay += calculate_expectArrivalDuration(trainData, distance, start->friction);
    }
    else {
        int distanceAC = 0;
        int distanceDC = 0;
        for(;;delay++) {
            double currentVelocity = calculate_currentVelocity(trainData, (timetickSinceSpeedChange + delay)) * (start->friction);
            distanceAC += currentVelocity;
            distanceDC = (int)(124.95 * currentVelocity - 22.769);
            if ((distanceAC + distanceDC) >= distance) {
                break;
            }
        }
    }
    return delay;
}
