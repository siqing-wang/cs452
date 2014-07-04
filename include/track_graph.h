#ifndef __TRACK_GRAPH_H__
#define __TRACK_GRAPH_H__

#include <track.h>

void trackGraph_init(struct TrainSetData *data, track_node *track);
void trackGraph_highlightSenPath(struct TrainSetData *data, track_node *node);
void trackGraph_unhighlightSenPath(struct TrainSetData *data, track_node *node);
void trackGraph_turnSw(struct TrainSetData *data, int switchNumber, int oldDir, int newDir);
#endif
