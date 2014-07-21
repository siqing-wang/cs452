#ifndef __TRACK_GRAPH_H__
#define __TRACK_GRAPH_H__

#include <track.h>

void trackGraph_initTrackA(struct TrainSetData *data, track_node *track);
void trackGraph_initTrackB(struct TrainSetData *data, track_node *track);

/* Standalone task functions. */
// Redraw train location AND UPDATE OLD POSITION VARIABLE IN PLACE.
void trackGraph_drawSw(TrainSetData *data, int swIndex, int dir);
void trackGraph_redrawTrainLoc(struct TrainSetData *data,
    unsigned int *oldGraphNodes, track_node *newn,
    int newoff, int *snapSwtable, int *coveredSwitch, int color);
void trackGraph_redrawSw(struct TrainSetData *data,
    int swIndex, int olddir, int newdir);
#endif
