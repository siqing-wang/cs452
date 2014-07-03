/* train_calibration.c - All data(speed/distance) related with train */

#include <train_calibration.h>

double calculate_trainVelocity(int trainNum, int speed) {
    if (speed == 0) {
        return 0;
    }

    double velocity = -1;
    if ((speed >= 1) && (speed <= 12)) {
        velocity =  0.52468 * speed - 0.24942;
    }
    else if (speed == 13) {
        velocity = 6.091978731;
    }
    else if (speed == 14) {
        velocity = 5.960918512;
    }
    switch(trainNum) {
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

double calculate_currentVelocity(TrainSetData *trainSetData, int trainIndex, int timetick) {
    TrainSpeedData *trainSpeedData = trainSetData->tstable[trainIndex];
    int trainNum = trainSpeedData->trainNum;
    int lastSpeed = trainSpeedData->lastSpeed;
    int targetSpeed = trainSpeedData->targetSpeed;

    double lastVelocity = calculate_trainVelocity(trainNum, lastSpeed);
    double targetVelocity = calculate_trainVelocity(trainNum, targetVelocity);

    if ((lastVelocity < 0) || (targetVelocity < 0)) {
        return -1;
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

double getCurrentVelocity(int distance, int timetick) {
    return (distance / timetick);
}

int calculate_delayToAchieveSpeed(TrainSetData *trainSetData, int trainIndex) {
    TrainSpeedData *trainSpeedData = trainSetData->tstable[trainIndex];
    int trainNum = trainSpeedData->trainNum;
    int lastSpeed = trainSpeedData->lastSpeed;
    int targetSpeed = trainSpeedData->targetSpeed;

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

int calculate_expectArrivalDuration(TrainSetData *trainSetData, int trainIndex, int distance, double restriction) {
    TrainSpeedData *trainSpeedData = trainSetData->tstable[trainIndex];
    int trainNum = trainSpeedData->trainNum;
    int targetSpeed = trainSpeedData->targetSpeed;
    int timetick = trainSpeedData->timetick;
    int timeRequired = trainSpeedData->timeRequiredToAchieveSpeed;

    double currentVelocity = 0;
    double currentDistance = 0;
    double targetVelocity = calculate_trainVelocity(trainNum, targetSpeed);

    int tick = 0;
    for(;;) {
        if (currentDistance >= distance) {
            break;
        }
        if ((timetick + tick) >= timeRequired) {
            break;
        }
        currentVelocity = calculate_currentVelocity(trainSetData, trainIndex, (timetick+ tick)) * restriction;
        currentDistance += currentVelocity;
        tick ++;
    }

    if (targetVelocity == 0) {
        return 0;
    }
    if (currentDistance < distance) {
        tick += ((distance - currentDistance) / (targetVelocity * restriction));
    }

    return tick;
}

int calculate_delayToStop(TrainSetData *trainSetData, int trainIndex, int distance) {
    TrainSpeedData *trainSpeedData = trainSetData->tstable[trainIndex];
    int trainNum = trainSpeedData->trainNum;
    int speed = trainSpeedData->targetSpeed;

    double minDistance = calculate_stopDistance(trainNum, speed);
    if (distance <= minDistance) {
        return -1;
    }

    return calculate_expectArrivalDuration(trainSetData, trainIndex, (distance - minDistance), 1);
}
