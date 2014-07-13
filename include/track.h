#ifndef __TRACK_H__
#define __TRACK_H__

#define TRACK_MAX 144
#define TRACK_SWITCH_NUM 22
#define TRACK_GRAPH_NODES_MAX 20

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

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int dist;             /* in millimetres */
  int numGraphNodes;
  unsigned int graphNodes[TRACK_GRAPH_NODES_MAX];
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
int nextDistance(struct TrainSetData *data, track_node *node);
int nextSensorDistance(struct TrainSetData *data, track_node *node);
int expectSensorArrivalTimeDuration(struct TrainSetData *data, int trainIndex, track_node *node, double friction);
int findRouteDistance(track_node *start, track_node *end, int *result, int resultIndex);
void init_tracka(track_node *track);
void init_trackb(track_node *track);

#endif
