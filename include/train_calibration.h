/*
 * train_calibration.h
 *  All data(speed/distance) related with train
 */

#ifndef __TRAIN_CALIBRATION_H__
#define __TRAIN_CALIBRATION_H__

#include <trainset.h>
#include <track.h>

double calculate_currentVelocity(TrainData *trainData, int timetick);
int calculate_delayToAchieveSpeed(TrainData *trainData);
int calculate_expectArrivalDuration(TrainData *trainData, int distance, double friction);
int calculate_expectTravelledDistance(TrainData *trainData, double friction);
int calculate_delayToStop(TrainSetData *trainSetData, TrainData *trainData, track_node *start, int distance);

#endif
