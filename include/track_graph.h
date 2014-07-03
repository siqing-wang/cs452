#ifndef __TRACK_GRAPH_H__
#define __TRACK_GRAPH_H__

#include <track.h>

void trackGraph_init(track_node *track);
void trackGraph_colorEdge(track_edge *edge, int color);
void trackGraph_colorTillNextSensor(struct TrainSetData *data, track_node *node, int color);

#endif
