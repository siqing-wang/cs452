/* train_calibration.c - All data(speed/distance) related with train */

#include <train_calibration.h>
#include <syscall.h>
#include <utils.h>

double calculate_trainVelocity(int trainNum, int speed) {
    if (speed == 0) {
        return 0;
    }

    double velocity = -1;
    if (speed == 1) {
        velocity = 0.140815433;
    }
    if ((speed >= 2) && (speed <= 12)) {
        velocity =  0.53679 * speed + 0.23677;
    }
    else if (speed == 13) {
        velocity = 6.091978731;
    }
    else if (speed == 14) {
        velocity = 5.960918512;
    }
    switch(trainNum) {
        case 45:
            velocity = velocity * 0.93;
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
                    return 477.875;
                case 9:
                    return 535.625;
                case 10:
                    return 591.125;
                case 11:
                    return 656.625;
                case 12:
                    return 719.125;
                case 13:
                    return 817.375;
                case 14:
                    return 801;
            }
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

int calculate_delayToAchieveSpeed(TrainData *trainData) {
    int trainNum = trainData->trainNum;
    int lastSpeed = trainData->lastSpeed;
    int targetSpeed = trainData->targetSpeed;

    // if (trainNum == 45) {
    //     if (targetSpeed == 0) {
    //         switch (lastSpeed) {
    //             case 8:
    //                 return 311;
    //             case 9:
    //                 return 311;
    //             case 10:
    //                 return 298;
    //             case 11:
    //                 return 316;
    //             case 12:
    //                 return 319;
    //         }
    //     }
    // }
    // if (trainNum == 49) {
    //     if (targetSpeed == 0) {
    //         switch (lastSpeed) {
    //             case 8:
    //                 return 294;
    //             case 9:
    //                 return 297;
    //             case 10:
    //                 return 299;
    //             case 11:
    //                 return 298;
    //             case 12:
    //                 return 304;
    //         }
    //     }
    // }

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

double calculate_currentVelocity(TrainData *trainData, int timetick) {
    int trainNum = trainData->trainNum;
    int lastSpeed = trainData->lastSpeed;
    int targetSpeed = trainData->targetSpeed;
    int delayRequiredToAchieveSpeed = trainData->delayRequiredToAchieveSpeed;

    double lastVelocity = calculate_trainVelocity(trainNum, lastSpeed);
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);

    if ((lastVelocity < 0) || (targetVelocity < 0)) {
        return -1;
    }

    if (timetick == 0) {
        return lastVelocity;
    }
    else if (timetick >= delayRequiredToAchieveSpeed) {
        return targetVelocity;
    }

    double distance = calculate_stopDistance(trainNum, targetSpeed) - calculate_stopDistance(trainNum, lastSpeed);
    if (distance < 0) {
        distance = 0 - distance;
    }

    double velocity1 = lastVelocity;
    double velocityDiff = targetVelocity - lastVelocity;
    double temp = timetick / delayRequiredToAchieveSpeed;
    velocity1 += 3 * velocityDiff * temp * temp;
    velocity1 -= 2 * velocityDiff * temp * temp * temp;

    // double velocity2 = lastVelocity;
    // int timeDiff = calculate_delayToAchieveSpeed(trainData);
    // temp = 1.0 * timetick / timeDiff;
    // velocity2 += (30 * distance - (12 * targetVelocity + 18 * lastVelocity) * timeDiff) * temp * temp / timeDiff;
    // velocity2 -= (60 * distance - (28 * targetVelocity + 32 * lastVelocity) * timeDiff) * temp * temp * temp / timeDiff;
    // velocity2 += (30 * distance - (15 * targetVelocity + 15 * lastVelocity) * timeDiff) * temp * temp * temp * temp / timeDiff;

    // return (velocity1 + velocity2)/2;
    return velocity1;
}

int calculate_expectArrivalDuration(TrainData *trainData, int distance, double friction) {
    // This will be calculated only when changing speed or trigger sensor
    // In both case, timetickSinceSpeedChange = timetickWhenHittingSensor

    int trainNum = trainData->trainNum;
    int targetSpeed = trainData->targetSpeed;
    int timetickSinceSpeedChange = trainData->timetickSinceSpeedChange;
    int delayRequiredToAchieveSpeed = trainData->delayRequiredToAchieveSpeed;

    double currentVelocity = 0;
    double currentDistance = 0;
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);

    currentDistance = 0;

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

int calculate_delayToStop(TrainSetData *trainSetData, TrainData *trainData, track_node *start, int distance) {
    int trainNum = trainData->trainNum;
    int targetSpeed = trainData->targetSpeed;
    int timetickSinceSpeedChange = trainData->timetickSinceSpeedChange;

    double minDistance = calculate_stopDistance(trainNum, targetSpeed);
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);
    double currentDistance = 0;
    double currentVelocity = 0;

    int delay = 0;
    if (distance >= (2 * minDistance)) {
        distance -= minDistance;

        for(;;) {
            if ((trainData->timetickSinceSpeedChange + delay) >= trainData->delayRequiredToAchieveSpeed) {
                break;
            }
            currentVelocity = calculate_currentVelocity(trainData, (trainData->timetickSinceSpeedChange + delay)) * (nextSensorOrExit(trainSetData, start)->friction);
            currentDistance += currentVelocity;
            delay ++;
        }

        distance -= currentDistance;
        assert((distance >= 0), "calculated_delayToStop : distance is negative after reach full speed.");

        for(;;) {
            if (start->type == NODE_EXIT) {
                break;
            }
            int passed = nextSensorDistance(trainSetData, start);
            if (passed < currentDistance) {
                currentDistance -= passed;
                start = nextSensorOrExit(trainSetData, start);
                continue;
            }

            passed -= currentDistance;
            currentDistance = 0;

            if (distance < passed) {
                break;
            }
            start = nextSensorOrExit(trainSetData, start);
            distance = distance - passed;
            delay += passed / (targetVelocity * start->friction);
        }
        delay += distance / (targetVelocity * start->friction);
    }
    else {
        int distanceAC = 0;
        int distanceDC = 0;
        for(;;delay++) {
            double currentVelocity = calculate_currentVelocity(trainData, (timetickSinceSpeedChange + delay)) * (start->friction);
            distanceAC += currentVelocity;
            switch(trainNum) {
                case 45:
                    distanceDC = (int)(135.18 * currentVelocity - 42.346);
                    break;
                case 49:
                default:
                    distanceDC = (int)(124.41 * currentVelocity - 26.838);
                    break;
            }

            if ((distanceAC + distanceDC) >= distance) {
                break;
            }
        }
    }
    return delay;
}
