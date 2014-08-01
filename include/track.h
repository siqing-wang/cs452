#ifndef __TRACK_H__
#define __TRACK_H__

#define TRACK_MAX 144
#define TRACK_SWITCH_NUM 22
#define TRACK_GRAPH_NODES_MAX 20

#include <trainset.h>

/* Data structures */
typedef enum {
  NODE_NONE,
  NODE_SENSOR,
  NODE_BRANCH,
  NODE_MERGE,
  NODE_ENTER,
  NODE_EXIT,
} node_type;

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1
#define DIR_REVERSE 2

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int dist;             /* in millimetres */
  int numGraphNodes;
  unsigned int graphNodes[TRACK_GRAPH_NODES_MAX];
  unsigned int reservation[TRAIN_NUM];
};

struct track_node {
  const char *name;
  node_type type;
  int num;              /* sensor or switch number */
  track_node *reverse;  /* same location, but opposite direction */
  track_edge edge[2];
  double friction;
  int visited;
};

/* Functions. */
struct TrainSetData;
track_node *nextNode(struct TrainSetData *data, track_node *node);
track_node *nextSensorOrExit(struct TrainSetData *data, track_node *node);
track_node *nextBranchOrExit(struct TrainSetData *data, track_node *node);
track_node *nextWrongDirSensorOrExit(struct TrainSetData *data, track_node *node);
int nextDistance(struct TrainSetData *data, track_node *node);
int nextSensorDistance(struct TrainSetData *data, track_node *node);
void fixBrokenSensor(struct TrainSetData *data, track_node *sensor);
void fixBrokenSwitch(struct TrainSetData *data, track_node *sw);
int isRouteBlocked(TrainSetData *data, track_edge *edge, int trainIndex, int low, int high);
int findRouteDistance(TrainSetData *data, int trainIndex, track_node *start, track_node *end, track_node *end_alt, int endOffset, track_node *lastNode, int *result, int resultIndex);
void init_tracka(track_node *track);
void init_trackb(track_node *track);

/* Track Reservation */
#define RESERV_SAFE_MARGIN 30
#define RESERV_MERGE_SAFE_MARGIN 260
#define RESERV_DELAY 10

/* Helpers. */
int reserv_checkReservation(track_edge *edge, int low, int high);
void reserv_getReservation(track_edge *edge, int trainIndex, int *low, int *high);
int reserv_isReserved(track_edge *edge, int trainIndex);
track_edge* reserv_getReservedEdge(track_node *node, int trainIndex);
void reserv_reserve(track_edge *edge, int trainIndex, int low, int high);
void reserv_clearReservation(track_edge *edge, int trainIndex);

/* Usage. */
void reserv_init(TrainSetData *data, int trainIndex);
int reserv_updateReservation(int trainCtrlTid, TrainSetData *data, int trainIndex);

/* Debug Printing. */
void printMyReservationRange(TrainSetData *data, int trainIndex);
#endif
