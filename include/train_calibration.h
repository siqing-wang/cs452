/*
 * train_calibration.h
 *  All data(speed/distance) related with train
 */

#ifndef __TRAIN_CALIBRATION_H__
#define __TRAIN_CALIBRATION_H__

#include <trainset.h>

double calculate_currentVelocity(TrainSetData *trainSetData, int trainIndex, int timetick);
double getCurrentVelocity(int distance, int timetick);
int calculate_delayToAchieveSpeed(TrainSetData *trainSetData, int trainIndex);
int calculate_expectArrivalDuration(TrainSetData *trainSetData, int trainIndex, int distance, double restriction);
int calculate_delayToStop(TrainSetData *trainSetData, int trainIndex, int distance);

#endif
