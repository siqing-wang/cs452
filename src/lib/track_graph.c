#include <track_graph.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <trainset.h>

#define SW_HIGHLIGHT_COLOR 32
#define PATH_HIGHLIGHT_COLOR 31
#define UNHIGHLIGHT_COLOR 37


/* Have each graph node's info inside a signle int to save space. */
void trackGraph_extractNodeInfo(unsigned int graphNodeInfo, int *row, int *col);
unsigned int trackGraph_buildNode(int row, int col);

/* Initialization Helper. */
void trackGraph_copyGraphNodesToReverse(track_edge *edge);


/* Standalone task functions. */
void trackGraph_drawNode(unsigned int node) {
    int row, col;
    trackGraph_extractNodeInfo(node, &row, &col);
    assert(row >= 0 && col >= 0, "trackGraph_drawNode: invalid node");
    PrintfAt(COM2, TRACK_R+row, TRACK_C+col, "*");
}

void trackGraph_drawSw(TrainSetData *data, int swIndex, int dir) {
    int indexInTrack = swIndex * 2 + 80;
    track_node *node = data->track + indexInTrack;

    assert(node->type == NODE_BRANCH, "trackGraph_drawSw: not a branch");
    PrintfAt(COM2, 0, 0, "\033[%dm", SW_HIGHLIGHT_COLOR);
    trackGraph_drawNode(node->reverse->edge[0].graphNodes[0]);
    trackGraph_drawNode(node->reverse->edge[0].graphNodes[1]);
    trackGraph_drawNode(node->edge[dir].graphNodes[1]);
    PutStr(COM2, TCS_RESET);
    IOidle(COM2);
}

void trackGraph_redrawTrainLoc(TrainSetData *data, unsigned int *oldGraphNodes, track_node *newn,
    int newoff, int *snapSwtable, int *coveredSwitch, int color) {

    if (newoff < 0) {
        /* Offset can be -ve when turning diretcion, ignore this case for simplicity. */
        return;
    }

    int i, dir;

    /* Get new position. */
    int newGraphNodes[3];

    /* Adjust offset (forward) get track edge. */
    track_edge *edge;
    for (;;) {

        if (newn->type == NODE_BRANCH) {
            dir = snapSwtable[getSwitchIndex(newn->num)];
        } else {
            dir = DIR_AHEAD;
        }
        edge = &(newn->edge[dir]);

        if (newoff < edge->dist) {
            break;
        }

        /* Move forward one edge. */
        newoff -= edge->dist;
        newn = edge->dest;
    }

    if (newn->type == NODE_EXIT) {
        /* Cannot draw beyond exit. */
        return;
    }

    /* Find leading node. */
    double ratio = (double)newoff / (double)edge->dist;
    int nodeIndex = (int)((double)(edge->numGraphNodes) * ratio);
    assert(nodeIndex < edge->numGraphNodes, "trackGraph_redrawTrainLoc: invalid node.");
    newGraphNodes[0] = edge->graphNodes[nodeIndex];

    /* Draw 2 nodes ahead. */
    track_node *node = newn;
    for (i = 1; i < 3; i++) {

        nodeIndex++;
        if (nodeIndex >= edge->numGraphNodes) {
            node = edge->dest;
            /* Go to last edge. */
            if (node->type == NODE_BRANCH) {
                dir = snapSwtable[getSwitchIndex(node->num)];
            } else if (node->type == NODE_EXIT) {
                return;
            } else {
                dir = DIR_AHEAD;
            }
            /* Turn back to correct direction. */
            edge = &(node->edge[dir]);
            nodeIndex = 1;
        }
        newGraphNodes[i] = edge->graphNodes[nodeIndex];
    }


    /* Draw new nodes. */
    PrintfAt(COM2, 0, 0, "\033[%dm", color);
    unsigned int graphNode = 0;
    for (i = 0; i < 3; i++) {
        graphNode = newGraphNodes[i];
        if (graphNode == oldGraphNodes[0] || graphNode == oldGraphNodes[1] || graphNode == oldGraphNodes[2]) {
            continue;
        }
        trackGraph_drawNode(graphNode);
    }

    /* Undraw old nodes and update them. */
    if (*oldGraphNodes >= 0) {
        PrintfAt(COM2, 0, 0, "\033[%dm", UNHIGHLIGHT_COLOR);
        for (i = 0; i < 3; i++) {
            graphNode = oldGraphNodes[i];
            if (graphNode == newGraphNodes[0] || graphNode == newGraphNodes[1] || graphNode == newGraphNodes[2]) {
                continue;
            }
            trackGraph_drawNode(graphNode);
        }
    }

    oldGraphNodes[0] = newGraphNodes[0];
    oldGraphNodes[1] = newGraphNodes[1];
    oldGraphNodes[2] = newGraphNodes[2];

    /* Redraw switch if necessary. */
    if (*coveredSwitch >= 0) {
        trackGraph_drawSw(data, *coveredSwitch, snapSwtable[*coveredSwitch]);
    }

    /* If we need to redraw switch next time. */
    *coveredSwitch = newn->type == NODE_BRANCH || newn->type == NODE_MERGE ?
        getSwitchIndex(newn->num) : -1;

    PutStr(COM2, TCS_RESET);
    IOidle(COM2);
}


void trackGraph_redrawSw(struct TrainSetData *data, int swIndex, int olddir, int newdir) {
    if (olddir == newdir) {
        return;
    }

    /* Get node for corresponding branch. */
    int indexInTrack = swIndex * 2 + 80;
    track_node *node = data->track + indexInTrack;

    assert(node->type == NODE_BRANCH, "trackGraph_redrawSw: not a branch");

    /* Undraw old. */
    PrintfAt(COM2, 0, 0, "\033[%dm", UNHIGHLIGHT_COLOR);
    trackGraph_drawNode(node->edge[olddir].graphNodes[1]);

    /* Draw new. */
    PrintfAt(COM2, 0, 0, "\033[%dm", SW_HIGHLIGHT_COLOR);
    trackGraph_drawNode(node->edge[newdir].graphNodes[1]);
    PutStr(COM2, TCS_RESET);

    IOidle(COM2);
}

void trackGraph_initTrackA(struct TrainSetData *data, track_node *track) {

    /* Draw Track. */
    PrintfAt(COM2, TRACK_R, TRACK_C,     "************************************************"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+1, TRACK_C,   "             * 12  *  11                          *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+2, TRACK_C,   "************     *  *****************************  *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+3, TRACK_C,   "          * 4   **        13 *         * 10       * *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+4, TRACK_C,   " ********      *               *  *  *              **"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+5, TRACK_C,   "              * 14              * * *               9 *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+6, TRACK_C,   "              *              156 *** 155              *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+7, TRACK_C,   "              *                   *                   *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+8, TRACK_C,   "              *              153 *** 154              *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+9, TRACK_C,   "              *                 * * *               8 *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+10, TRACK_C,  " ********      * 15            *  *  *              **"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+11, TRACK_C,  "         * 1    **        16 *         * 17       * *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+12, TRACK_C,  " **********      *  *****************************  *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+13, TRACK_C,  "           *  2    *     6                  7     *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+14, TRACK_C,  "**************        **************************"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+15, TRACK_C,  "               *  3        * 18         5 *"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+16, TRACK_C,  "******************************************************"); IOidle(COM2);

    // A1 -> 12
    track[0].edge[DIR_AHEAD].numGraphNodes = 5;
    track[0].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(0,11);
    track[0].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(0,12);
    track[0].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(0,13);
    track[0].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(0,14);
    track[0].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(0,15);
    trackGraph_copyGraphNodesToReverse(&(track[0].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[0].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A2 -> EX5
    track[1].edge[DIR_AHEAD].numGraphNodes=12;
    track[1].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,11);
    track[1].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,10);
    track[1].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,9);
    track[1].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,8);
    track[1].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,7);
    track[1].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,6);
    track[1].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,5);
    track[1].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,4);
    track[1].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,3);
    track[1].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,2);
    track[1].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,1);
    track[1].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,0);
    trackGraph_copyGraphNodesToReverse(&(track[1].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[1].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A3 -> 14
    track[2].edge[DIR_AHEAD].numGraphNodes=2;
    track[2].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,14);
    track[2].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,15);
    trackGraph_copyGraphNodesToReverse(&(track[2].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[2].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A4 -> B16
    track[3].edge[DIR_AHEAD].numGraphNodes=5;
    track[3].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,14);
    track[3].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,14);
    track[3].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,14);
    track[3].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,14);
    track[3].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,14);
    trackGraph_copyGraphNodesToReverse(&(track[3].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[3].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A5 -> 3   to 16
    track[4].edge[DIR_AHEAD].numGraphNodes=6;
    track[4].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,13);
    track[4].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,14);
    track[4].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,15);
    track[4].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,16);
    track[4].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,17);
    track[4].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,18);
    trackGraph_copyGraphNodesToReverse(&(track[4].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[4].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A6 -> B10
    track[5].edge[DIR_AHEAD].numGraphNodes=13;
    track[5].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,13);
    track[5].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,12);
    track[5].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,11);
    track[5].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,10);
    track[5].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,9);
    track[5].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,8);
    track[5].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,7);
    track[5].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,6);
    track[5].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,5);
    track[5].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,4);
    track[5].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,3);
    track[5].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,2);
    track[5].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(16,1);
    trackGraph_copyGraphNodesToReverse(&(track[5].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[5].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A7 -> B12
    track[6].edge[DIR_AHEAD].numGraphNodes=10;
    track[6].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10);
    track[6].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,9);
    track[6].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,8);
    track[6].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,7);
    track[6].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,6);
    track[6].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,5);
    track[6].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,4);
    track[6].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,3);
    track[6].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(14,2);
    track[6].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(14,1);
    trackGraph_copyGraphNodesToReverse(&(track[6].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[6].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A8 -> 2
    track[7].edge[DIR_AHEAD].numGraphNodes=4;
    track[7].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10);
    track[7].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,11);
    track[7].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,12);
    track[7].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,13);
    trackGraph_copyGraphNodesToReverse(&(track[7].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[7].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A9 -> B8
    track[8].edge[DIR_AHEAD].numGraphNodes=6;
    track[8].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,7);
    track[8].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,6);
    track[8].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,5);
    track[8].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,4);
    track[8].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,3);
    track[8].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,2);
    trackGraph_copyGraphNodesToReverse(&(track[8].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[8].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A10 -> 1
    track[9].edge[DIR_AHEAD].numGraphNodes=4;
    track[9].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,7);
    track[9].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,8);
    track[9].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,9);
    track[9].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,10);
    trackGraph_copyGraphNodesToReverse(&(track[9].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[9].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A11 -> 1
    track[10].edge[DIR_AHEAD].numGraphNodes=9;
    track[10].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,2);
    track[10].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,3);
    track[10].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,4);
    track[10].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(10,5);
    track[10].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,6);
    track[10].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(10,7);
    track[10].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(10,8);
    track[10].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(11,9);
    track[10].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(12,10);
    trackGraph_copyGraphNodesToReverse(&(track[10].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[10].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A12 -> EX
    track[11].edge[DIR_AHEAD].numGraphNodes=2;
    track[11].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,2);
    track[11].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,1);
    trackGraph_copyGraphNodesToReverse(&(track[11].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[11].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A13 -> 4
    track[12].edge[DIR_AHEAD].numGraphNodes = 4;
    track[12].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(2, 8);
    track[12].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(2, 9);
    track[12].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(2, 10);
    track[12].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(2, 11);
    trackGraph_copyGraphNodesToReverse(&(track[12].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[12].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A14 -> EX
    track[13].edge[DIR_AHEAD].numGraphNodes = 9;
    track[13].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(2, 8);
    track[13].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(2, 7);
    track[13].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(2, 6);
    track[13].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(2, 5);
    track[13].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(2, 4);
    track[13].edge[DIR_AHEAD].graphNodes[5] = trackGraph_buildNode(2, 3);
    track[13].edge[DIR_AHEAD].graphNodes[6] = trackGraph_buildNode(2, 2);
    track[13].edge[DIR_AHEAD].graphNodes[7] = trackGraph_buildNode(2, 1);
    track[13].edge[DIR_AHEAD].graphNodes[8] = trackGraph_buildNode(2, 0);
    trackGraph_copyGraphNodesToReverse(&(track[13].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[13].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A15 -> EX
    track[14].edge[DIR_AHEAD].numGraphNodes = 5;
    track[14].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(4, 5);
    track[14].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(4, 4);
    track[14].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(4, 3);
    track[14].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(4, 2);
    track[14].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(4, 1);
    trackGraph_copyGraphNodesToReverse(&(track[14].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[14].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A16 -> 4
    track[15].edge[DIR_AHEAD].numGraphNodes=6;
    track[15].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,5);
    track[15].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,6);
    track[15].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,7);
    track[15].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,8);
    track[15].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(3,10);
    track[15].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,11);
    trackGraph_copyGraphNodesToReverse(&(track[15].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[15].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B1 -> D14
    track[16].edge[DIR_AHEAD].numGraphNodes=9;
    track[16].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,30);
    track[16].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,31);
    track[16].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,32);
    track[16].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,33);
    track[16].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,34);
    track[16].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,35);
    track[16].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(12,36);
    track[16].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(12,37);
    track[16].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(12,38);
    trackGraph_copyGraphNodesToReverse(&(track[16].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[16].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B2 -> 16
    track[17].edge[DIR_AHEAD].numGraphNodes=4;
    track[17].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,30);
    track[17].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,29);
    track[17].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,28);
    track[17].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,27);
    trackGraph_copyGraphNodesToReverse(&(track[17].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[17].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B3 -> C2
    track[18].edge[DIR_AHEAD].numGraphNodes=3;
    track[18].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,29);
    track[18].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,31);
    track[18].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,32);
    trackGraph_copyGraphNodesToReverse(&(track[18].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[18].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B4 -> 16
    track[19].edge[DIR_AHEAD].numGraphNodes=2;
    track[19].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,29);
    track[19].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,27);
    trackGraph_copyGraphNodesToReverse(&(track[19].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[19].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B5 -> D3
    track[20].edge[DIR_AHEAD].numGraphNodes=9;
    track[20].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,30);
    track[20].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,31);
    track[20].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,32);
    track[20].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,33);
    track[20].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,34);
    track[20].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,35);
    track[20].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(2,36);
    track[20].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(2,37);
    track[20].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(2,38);
    trackGraph_copyGraphNodesToReverse(&(track[20].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[20].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B6 -> 13
    track[21].edge[DIR_AHEAD].numGraphNodes=4;
    track[21].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,30);
    track[21].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,29);
    track[21].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,28);
    track[21].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,27);
    trackGraph_copyGraphNodesToReverse(&(track[21].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[21].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B8 -> EX
    track[23].edge[DIR_AHEAD].numGraphNodes=2;
    track[23].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,2);
    track[23].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,1);
    trackGraph_copyGraphNodesToReverse(&(track[23].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[23].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B10 -> EX
    track[25].edge[DIR_AHEAD].numGraphNodes=2;
    track[25].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,1);
    track[25].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,0);
    trackGraph_copyGraphNodesToReverse(&(track[25].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[25].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B12 -> EX
    track[27].edge[DIR_AHEAD].numGraphNodes=2;
    track[27].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,1);
    track[27].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,0);
    trackGraph_copyGraphNodesToReverse(&(track[27].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[27].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B13 -> 154
    track[28].edge[DIR_AHEAD].numGraphNodes=3;
    track[28].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,36);
    track[28].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,35);
    track[28].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[28].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[28].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B14 -> D16
    track[29].edge[DIR_AHEAD].numGraphNodes=3;
    track[29].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,36);
    track[29].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,37);
    track[29].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(11,39);
    trackGraph_copyGraphNodesToReverse(&(track[29].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[29].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B16 -> 15
    track[31].edge[DIR_AHEAD].numGraphNodes=2;
    track[31].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,14);
    track[31].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,15);
    trackGraph_copyGraphNodesToReverse(&(track[31].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[31].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C2 -> 153
    track[33].edge[DIR_AHEAD].numGraphNodes=3;
    track[33].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,32);
    track[33].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,33);
    track[33].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[33].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[33].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C3 -> EX3
    track[34].edge[DIR_AHEAD].numGraphNodes=9;
    track[34].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,45);
    track[34].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,46);
    track[34].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,47);
    track[34].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,48);
    track[34].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,49);
    track[34].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,50);
    track[34].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,51);
    track[34].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,52);
    track[34].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,53);
    trackGraph_copyGraphNodesToReverse(&(track[34].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[34].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C4 -> 5
    track[35].edge[DIR_AHEAD].numGraphNodes=6;
    track[35].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,45);
    track[35].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,44);
    track[35].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,43);
    track[35].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,42);
    track[35].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,41);
    track[35].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,40);
    trackGraph_copyGraphNodesToReverse(&(track[35].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[35].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C5 -> 6
    track[36].edge[DIR_AHEAD].numGraphNodes=4;
    track[36].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,22);
    track[36].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,23);
    track[36].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,24);
    track[36].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,25);
    trackGraph_copyGraphNodesToReverse(&(track[36].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[36].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C6 -> 15
    track[37].edge[DIR_AHEAD].numGraphNodes=5;
    track[37].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,22);
    track[37].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,19);
    track[37].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,17);
    track[37].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,16);
    track[37].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,15);
    trackGraph_copyGraphNodesToReverse(&(track[37].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[37].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C7 -> 18
    track[38].edge[DIR_AHEAD].numGraphNodes=6;
    track[38].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,24);
    track[38].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,25);
    track[38].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,26);
    track[38].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,27);
    track[38].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,28);
    track[38].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,29);
    trackGraph_copyGraphNodesToReverse(&(track[38].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[38].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C8 -> 3
    track[39].edge[DIR_AHEAD].numGraphNodes=7;
    track[39].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,24);
    track[39].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,23);
    track[39].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,22);
    track[39].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,21);
    track[39].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,20);
    track[39].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,19);
    track[39].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,18);
    trackGraph_copyGraphNodesToReverse(&(track[39].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[39].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C9 -> 15
    track[40].edge[DIR_AHEAD].numGraphNodes=5;
    track[40].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,22);
    track[40].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,21);
    track[40].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,20);
    track[40].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,17);
    track[40].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,15);
    trackGraph_copyGraphNodesToReverse(&(track[40].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[40].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C10 -> 16
    track[41].edge[DIR_AHEAD].numGraphNodes=6;
    track[41].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,22);
    track[41].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,23);
    track[41].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,24);
    track[41].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,25);
    track[41].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,26);
    track[41].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,27);
    trackGraph_copyGraphNodesToReverse(&(track[41].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[41].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C11 -> 13
    track[42].edge[DIR_AHEAD].numGraphNodes=6;
    track[42].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,22);
    track[42].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,23);
    track[42].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,24);
    track[42].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,25);
    track[42].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,26);
    track[42].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,27);
    trackGraph_copyGraphNodesToReverse(&(track[42].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[42].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C12 -> 14
    track[43].edge[DIR_AHEAD].numGraphNodes=5;
    track[43].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,22);
    track[43].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,21);
    track[43].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,20);
    track[43].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(3,17);
    track[43].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,15);
    trackGraph_copyGraphNodesToReverse(&(track[43].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[43].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);


    // C13 -> E7
    track[44].edge[DIR_AHEAD].numGraphNodes=19;
    track[44].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25);
    track[44].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,26);
    track[44].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,27);
    track[44].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,28);
    track[44].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,29);
    track[44].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,30);
    track[44].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,31);
    track[44].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,32);
    track[44].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,33);
    track[44].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,34);
    track[44].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,35);
    track[44].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,36);
    track[44].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(0,37);
    track[44].edge[DIR_AHEAD].graphNodes[13]=trackGraph_buildNode(0,38);
    track[44].edge[DIR_AHEAD].graphNodes[14]=trackGraph_buildNode(0,39);
    track[44].edge[DIR_AHEAD].graphNodes[15]=trackGraph_buildNode(0,40);
    track[44].edge[DIR_AHEAD].graphNodes[16]=trackGraph_buildNode(0,41);
    track[44].edge[DIR_AHEAD].graphNodes[17]=trackGraph_buildNode(0,42);
    track[44].edge[DIR_AHEAD].graphNodes[18]=trackGraph_buildNode(0,43);
    trackGraph_copyGraphNodesToReverse(&(track[44].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[44].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C14 -> 11
    track[45].edge[DIR_AHEAD].numGraphNodes=4;
    track[45].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25);
    track[45].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,24);
    track[45].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,23);
    track[45].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,22);
    trackGraph_copyGraphNodesToReverse(&(track[45].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[45].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C15 -> D12
    track[46].edge[DIR_AHEAD].numGraphNodes=12;
    track[46].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,29);
    track[46].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,30);
    track[46].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,31);
    track[46].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,32);
    track[46].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,33);
    track[46].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,34);
    track[46].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,35);
    track[46].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,36);
    track[46].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(14,37);
    track[46].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(14,38);
    track[46].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(14,39);
    track[46].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(14,40);
    trackGraph_copyGraphNodesToReverse(&(track[46].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[46].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C16 -> 6
    track[47].edge[DIR_AHEAD].numGraphNodes=5;
    track[47].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,29);
    track[47].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,28);
    track[47].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,27);
    track[47].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,26);
    track[47].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,25);
    trackGraph_copyGraphNodesToReverse(&(track[47].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[47].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D1 -> 155
    track[48].edge[DIR_AHEAD].numGraphNodes=3;
    track[48].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,36);
    track[48].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,35);
    track[48].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[48].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[48].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D2 -> E4
    track[49].edge[DIR_AHEAD].numGraphNodes=3;
    track[49].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,36);
    track[49].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,37);
    track[49].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(3,39);
    trackGraph_copyGraphNodesToReverse(&(track[49].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[49].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D3 -> 10
    track[50].edge[DIR_AHEAD].numGraphNodes=4;
    track[50].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,38);
    track[50].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,39);
    track[50].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,40);
    track[50].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[50].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[50].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D5 -> E6
    track[52].edge[DIR_AHEAD].numGraphNodes=6;
    track[52].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,50);
    track[52].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,48);
    track[52].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,47);
    track[52].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,46);
    track[52].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,45);
    track[52].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,44);
    trackGraph_copyGraphNodesToReverse(&(track[52].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[52].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D6 -> 9
    track[53].edge[DIR_AHEAD].numGraphNodes=3;
    track[53].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,50);
    track[53].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,52);
    track[53].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(5,54);
    trackGraph_copyGraphNodesToReverse(&(track[53].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[53].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D7 -> 9
    track[54].edge[DIR_AHEAD].numGraphNodes=4;
    track[54].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,51);
    track[54].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,52);
    track[54].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,53);
    track[54].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,54);
    trackGraph_copyGraphNodesToReverse(&(track[54].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[54].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D8 -> E8
    track[55].edge[DIR_AHEAD].numGraphNodes=7;
    track[55].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,51);
    track[55].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,50);
    track[55].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,47);
    track[55].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,46);
    track[55].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,45);
    track[55].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,44);
    track[55].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,43);
    trackGraph_copyGraphNodesToReverse(&(track[55].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[55].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D9 -> E12
    track[56].edge[DIR_AHEAD].numGraphNodes=3;
    track[56].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,51);
    track[56].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,50);
    track[56].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,47);
    trackGraph_copyGraphNodesToReverse(&(track[56].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[56].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D10 -> 8
    track[57].edge[DIR_AHEAD].numGraphNodes=4;
    track[57].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,51);
    track[57].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(11,52);
    track[57].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,53);
    track[57].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,54);
    trackGraph_copyGraphNodesToReverse(&(track[57].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[57].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D12 -> 7
    track[59].edge[DIR_AHEAD].numGraphNodes=5;
    track[59].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,40);
    track[59].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,41);
    track[59].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,42);
    track[59].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,43);
    track[59].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,44);
    trackGraph_copyGraphNodesToReverse(&(track[59].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[59].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D14 -> 17
    track[61].edge[DIR_AHEAD].numGraphNodes=4;
    track[61].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,38);
    track[61].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,39);
    track[61].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,40);
    track[61].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,41);
    trackGraph_copyGraphNodesToReverse(&(track[61].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[61].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D16 -> 17
    track[63].edge[DIR_AHEAD].numGraphNodes=2;
    track[63].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,39);
    track[63].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,41);
    trackGraph_copyGraphNodesToReverse(&(track[63].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[63].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E1 -> 156
    track[64].edge[DIR_AHEAD].numGraphNodes=3;
    track[64].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,32);
    track[64].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,33);
    track[64].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[64].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[64].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E2 -> 15
    track[65].edge[DIR_AHEAD].numGraphNodes=3;
    track[65].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,32);
    track[65].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,31);
    track[65].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(3,29);
    trackGraph_copyGraphNodesToReverse(&(track[65].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[65].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E4 -> 10
    track[67].edge[DIR_AHEAD].numGraphNodes=2;
    track[67].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,39);
    track[67].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[67].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[67].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E6 -> 10
    track[69].edge[DIR_AHEAD].numGraphNodes=4;
    track[69].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44);
    track[69].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,43);
    track[69].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,42);
    track[69].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[69].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[69].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E9 -> 8
    track[72].edge[DIR_AHEAD].numGraphNodes=3;
    track[72].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,50);
    track[72].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,52);
    track[72].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,54);
    trackGraph_copyGraphNodesToReverse(&(track[72].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[72].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E10 -> E13
    track[73].edge[DIR_AHEAD].numGraphNodes=6;
    track[73].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,50);
    track[73].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,48);
    track[73].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,47);
    track[73].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,46);
    track[73].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,45);
    track[73].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,44);
    trackGraph_copyGraphNodesToReverse(&(track[73].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[73].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E12 -> 7
    track[75].edge[DIR_AHEAD].numGraphNodes=4;
    track[75].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,47);
    track[75].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,46);
    track[75].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,45);
    track[75].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,44);
    trackGraph_copyGraphNodesToReverse(&(track[75].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[75].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E13 -> 17
    track[76].edge[DIR_AHEAD].numGraphNodes=4;
    track[76].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,44);
    track[76].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,43);
    track[76].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,42);
    track[76].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,41);
    trackGraph_copyGraphNodesToReverse(&(track[76].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[76].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E15 -> 13
    track[78].edge[DIR_AHEAD].numGraphNodes=2;
    track[78].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,29);
    track[78].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,27);
    trackGraph_copyGraphNodesToReverse(&(track[78].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[78].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // MR1 -> 2
    track[81].edge[DIR_AHEAD].numGraphNodes=3;
    track[81].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,10);
    track[81].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,11);
    track[81].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,13);
    trackGraph_copyGraphNodesToReverse(&(track[81].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[81].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR2 -> 3
    track[83].edge[DIR_AHEAD].numGraphNodes=3;
    track[83].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,13);
    track[83].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,15);
    track[83].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,18);
    trackGraph_copyGraphNodesToReverse(&(track[83].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[83].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR4 -> 12
    track[87].edge[DIR_AHEAD].numGraphNodes=3;
    track[87].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,11);
    track[87].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,13);
    track[87].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,15);
    trackGraph_copyGraphNodesToReverse(&(track[87].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[87].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR5 -> 18
    track[89].edge[DIR_AHEAD].numGraphNodes=12;
    track[89].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,29);
    track[89].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,30);
    track[89].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,31);
    track[89].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,32);
    track[89].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,33);
    track[89].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,34);
    track[89].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,35);
    track[89].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,36);
    track[89].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,37);
    track[89].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,38);
    track[89].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,39);
    track[89].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,40);
    trackGraph_copyGraphNodesToReverse(&(track[89].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[89].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // BR6 CURVED-> 18
    track[90].edge[DIR_CURVED].numGraphNodes=3;
    track[90].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(14,25);
    track[90].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,27);
    track[90].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(16,29);
    trackGraph_copyGraphNodesToReverse(&(track[90].edge[DIR_CURVED]));
    //trackGraph_colorEdge(&(track[90].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // BR7 CURVED-> 5
    track[92].edge[DIR_CURVED].numGraphNodes=3;
    track[92].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(14,44);
    track[92].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,42);
    track[92].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(16,40);
    trackGraph_copyGraphNodesToReverse(&(track[92].edge[DIR_CURVED]));
    //trackGraph_colorEdge(&(track[92].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // MR9 -> MR8
    track[97].edge[DIR_AHEAD].numGraphNodes=5;
    track[97].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,54);
    track[97].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,54);
    track[97].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,54);
    track[97].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,54);
    track[97].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,54);
    trackGraph_copyGraphNodesToReverse(&(track[97].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[97].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // BR11 CURVEd-> 14
    track[100].edge[DIR_CURVED].numGraphNodes=5;
    track[100].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(0,22);
    track[100].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,19);
    track[100].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(2,17);
    track[100].edge[DIR_CURVED].graphNodes[3]=trackGraph_buildNode(3,16);
    track[100].edge[DIR_CURVED].graphNodes[4]=trackGraph_buildNode(4,15);
    trackGraph_copyGraphNodesToReverse(&(track[100].edge[DIR_CURVED]));
    //trackGraph_colorEdge(&(track[100].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // MR12 -> 11
    track[103].edge[DIR_AHEAD].numGraphNodes=8;
    track[103].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,15);
    track[103].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,16);
    track[103].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,17);
    track[103].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,18);
    track[103].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,19);
    track[103].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,20);
    track[103].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,21);
    track[103].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,22);
    trackGraph_copyGraphNodesToReverse(&(track[103].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[103].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR153 -> 154
    track[117].edge[DIR_AHEAD].numGraphNodes=2;
    track[117].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34);
    track[117].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[117].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR154 -> 156
    track[119].edge[DIR_AHEAD].numGraphNodes=2;
    track[119].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34);
    track[119].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[119].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR155 -> 156
    track[121].edge[DIR_AHEAD].numGraphNodes=2;
    track[121].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34);
    track[121].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[121].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR156 -> 154
    track[123].edge[DIR_AHEAD].numGraphNodes=2;
    track[123].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34);
    track[123].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[123].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // EN1 -> 155
    track[124].edge[DIR_AHEAD].numGraphNodes=4;
    track[124].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,34);
    track[124].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,34);
    track[124].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(8,34);
    track[124].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[124].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[126].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // EN2 -> 153
    track[126].edge[DIR_AHEAD].numGraphNodes=4;
    track[126].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,34);
    track[126].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(5,34);
    track[126].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(6,34);
    track[126].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[126].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[124].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    /* Initialize switch in graph. */
    // int i = 0;
    // for ( ; i < SWITCH_TOTAL; i++) {
    //     trackGraph_colorSwitch(&(track[i * 2 + 80].edge[data->swtable[i]]), SW_HIGHLIGHT_COLOR);
    // }
}

void trackGraph_initTrackB(struct TrainSetData *data, track_node *track) {

    /* Draw Track. */
    PrintfAt(COM2, TRACK_R, TRACK_C,     " ******************************************************"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+1, TRACK_C,   "            * 5         18 *         3 *               "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+2, TRACK_C,   "       **************************        **************"); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+3, TRACK_C,   "    *    7                   6     *    2  *           "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+4, TRACK_C,   "   *  *****************************  *       ********* "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+5, TRACK_C,   "  * *       17 *         * 16        **     1  x       "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+6, TRACK_C,   " **              *  *  *            15 *         *     "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+7, TRACK_C,   "* 8               * * *                 *         *    "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+8, TRACK_C,   "*              154 *** 153              *         *    "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+9, TRACK_C,   "*                   *                   *         *    "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+10, TRACK_C,  "*              155 *** 156              *         *    "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+11, TRACK_C,  "* 9               * * *                 *         *    "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+12, TRACK_C,  " **               *  *  *           14 *         *     "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+13, TRACK_C,  "  * *       10 *         * 13        **     4  *       "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+14, TRACK_C,  "   *  *****************************  *       ********* "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+15, TRACK_C,  "    *                          11  *  12 *             "); IOidle(COM2);
    PrintfAt(COM2, TRACK_R+16, TRACK_C,  "       ************************************************"); IOidle(COM2);

     // A1->12
    track[0].edge[DIR_AHEAD].numGraphNodes=4;
    track[0].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,40);
    track[0].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,39);
    track[0].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,38);
    track[0].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,37);
    trackGraph_copyGraphNodesToReverse(&(track[0].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[0].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A2->EX5
    track[1].edge[DIR_AHEAD].numGraphNodes=12;
    track[1].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,43);
    track[1].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,44);
    track[1].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,45);
    track[1].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,46);
    track[1].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,47);
    track[1].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,48);
    track[1].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,49);
    track[1].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,50);
    track[1].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,51);
    track[1].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,52);
    track[1].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,53);
    track[1].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,54);
    trackGraph_copyGraphNodesToReverse(&(track[1].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[1].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A3->14
    track[2].edge[DIR_AHEAD].numGraphNodes=2;
    track[2].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,40);
    track[2].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,39);
    trackGraph_copyGraphNodesToReverse(&(track[2].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[2].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A4->B16
    track[3].edge[DIR_AHEAD].numGraphNodes=5;
    track[3].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,40);
    track[3].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,40);
    track[3].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,40);
    track[3].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,40);
    track[3].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(7,40);
    trackGraph_copyGraphNodesToReverse(&(track[3].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[3].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A5->3to16
    track[4].edge[DIR_AHEAD].numGraphNodes=6;
    track[4].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,41);
    track[4].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,40);
    track[4].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,39);
    track[4].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,38);
    track[4].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,37);
    track[4].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,36);
    trackGraph_copyGraphNodesToReverse(&(track[4].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[4].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A6->B10
    track[5].edge[DIR_AHEAD].numGraphNodes=13;
    track[5].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,41);
    track[5].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,42);
    track[5].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,43);
    track[5].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,44);
    track[5].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,45);
    track[5].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,46);
    track[5].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,47);
    track[5].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,48);
    track[5].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,49);
    track[5].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,50);
    track[5].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,51);
    track[5].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,52);
    track[5].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(0,53);
    trackGraph_copyGraphNodesToReverse(&(track[5].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[5].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A7->B12
    track[6].edge[DIR_AHEAD].numGraphNodes=10;
    track[6].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44);
    track[6].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,45);
    track[6].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,46);
    track[6].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,47);
    track[6].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,48);
    track[6].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,49);
    track[6].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(2,50);
    track[6].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(2,51);
    track[6].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(2,52);
    track[6].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(2,53);
    trackGraph_copyGraphNodesToReverse(&(track[6].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[6].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A8->2
    track[7].edge[DIR_AHEAD].numGraphNodes=4;
    track[7].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44);
    track[7].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,43);
    track[7].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,42);
    track[7].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[7].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[7].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A9->B8
    track[8].edge[DIR_AHEAD].numGraphNodes=6;
    track[8].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,47);
    track[8].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,48);
    track[8].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,49);
    track[8].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,50);
    track[8].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,51);
    track[8].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,52);
    trackGraph_copyGraphNodesToReverse(&(track[8].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[8].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A10->1
    track[9].edge[DIR_AHEAD].numGraphNodes=3;
    track[9].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,47);
    track[9].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,46);
    track[9].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,45);
    trackGraph_copyGraphNodesToReverse(&(track[9].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[9].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A11->1
    track[10].edge[DIR_AHEAD].numGraphNodes=2;
    track[10].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,47);
    track[10].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,45);
    trackGraph_copyGraphNodesToReverse(&(track[10].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[10].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A12->A16
    track[11].edge[DIR_AHEAD].numGraphNodes=9;
    track[11].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,47);
    track[11].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,49);
    track[11].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,50);
    track[11].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,50);
    track[11].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,50);
    track[11].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(10,50);
    track[11].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(11,50);
    track[11].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(12,49);
    track[11].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(13,47);
    trackGraph_copyGraphNodesToReverse(&(track[11].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[11].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A13->4
    track[12].edge[DIR_AHEAD].numGraphNodes=3;
    track[12].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,46);
    track[12].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,45);
    track[12].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,44);
    trackGraph_copyGraphNodesToReverse(&(track[12].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[12].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A14->EX
    track[13].edge[DIR_AHEAD].numGraphNodes=8;
    track[13].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,46);
    track[13].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,47);
    track[13].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,48);
    track[13].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,49);
    track[13].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,50);
    track[13].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,51);
    track[13].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,52);
    track[13].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,53);
    trackGraph_copyGraphNodesToReverse(&(track[13].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[13].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A16->4
    track[15].edge[DIR_AHEAD].numGraphNodes=2;
    track[15].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,47);
    track[15].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,44);
    trackGraph_copyGraphNodesToReverse(&(track[15].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[15].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B1->D14
    track[16].edge[DIR_AHEAD].numGraphNodes=9;
    track[16].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,24);
    track[16].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,23);
    track[16].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,22);
    track[16].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,21);
    track[16].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,20);
    track[16].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,19);
    track[16].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(4,18);
    track[16].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(4,17);
    track[16].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(4,16);
    trackGraph_copyGraphNodesToReverse(&(track[16].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[16].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B2->16
    track[17].edge[DIR_AHEAD].numGraphNodes=4;
    track[17].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,24);
    track[17].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,25);
    track[17].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,26);
    track[17].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,27);
    trackGraph_copyGraphNodesToReverse(&(track[17].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[17].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B3->C2
    track[18].edge[DIR_AHEAD].numGraphNodes=3;
    track[18].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,25);
    track[18].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,23);
    track[18].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,22);
    trackGraph_copyGraphNodesToReverse(&(track[18].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[18].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B4->16
    track[19].edge[DIR_AHEAD].numGraphNodes=2;
    track[19].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,25);
    track[19].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,27);
    trackGraph_copyGraphNodesToReverse(&(track[19].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[19].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B5->D3
    track[20].edge[DIR_AHEAD].numGraphNodes=9;
    track[20].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,24);
    track[20].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,23);
    track[20].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,22);
    track[20].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,21);
    track[20].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,20);
    track[20].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,19);
    track[20].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,18);
    track[20].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,17);
    track[20].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(14,16);
    trackGraph_copyGraphNodesToReverse(&(track[20].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[20].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B6->13
    track[21].edge[DIR_AHEAD].numGraphNodes=4;
    track[21].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,24);
    track[21].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,25);
    track[21].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,26);
    track[21].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,27);
    trackGraph_copyGraphNodesToReverse(&(track[21].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[21].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B8->EX
    track[23].edge[DIR_AHEAD].numGraphNodes=2;
    track[23].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,52);
    track[23].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,53);
    trackGraph_copyGraphNodesToReverse(&(track[23].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[23].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B10->EX
    track[25].edge[DIR_AHEAD].numGraphNodes=2;
    track[25].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,53);
    track[25].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,54);
    trackGraph_copyGraphNodesToReverse(&(track[25].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[25].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B12->EX
    track[27].edge[DIR_AHEAD].numGraphNodes=2;
    track[27].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,53);
    track[27].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,54);
    trackGraph_copyGraphNodesToReverse(&(track[27].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[27].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B13->154
    track[28].edge[DIR_AHEAD].numGraphNodes=3;
    track[28].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,18);
    track[28].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,19);
    track[28].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[28].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[28].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B14->D16
    track[29].edge[DIR_AHEAD].numGraphNodes=3;
    track[29].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,18);
    track[29].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,17);
    track[29].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(5,15);
    trackGraph_copyGraphNodesToReverse(&(track[29].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[29].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B16->15
    track[31].edge[DIR_AHEAD].numGraphNodes=2;
    track[31].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,40);
    track[31].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,39);
    trackGraph_copyGraphNodesToReverse(&(track[31].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[31].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C2->153
    track[33].edge[DIR_AHEAD].numGraphNodes=3;
    track[33].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,22);
    track[33].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,21);
    track[33].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[33].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[33].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C3->EX3
    track[34].edge[DIR_AHEAD].numGraphNodes=9;
    track[34].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,9);
    track[34].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,8);
    track[34].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,7);
    track[34].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,6);
    track[34].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,5);
    track[34].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,4);
    track[34].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,3);
    track[34].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,2);
    track[34].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,1);
    trackGraph_copyGraphNodesToReverse(&(track[34].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[34].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C4->5
    track[35].edge[DIR_AHEAD].numGraphNodes=6;
    track[35].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,9);
    track[35].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,10);
    track[35].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,11);
    track[35].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,12);
    track[35].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,13);
    track[35].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,14);
    trackGraph_copyGraphNodesToReverse(&(track[35].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[35].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C5->6
    track[36].edge[DIR_AHEAD].numGraphNodes=4;
    track[36].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,32);
    track[36].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,31);
    track[36].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,30);
    track[36].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,29);
    trackGraph_copyGraphNodesToReverse(&(track[36].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[36].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C6->15
    track[37].edge[DIR_AHEAD].numGraphNodes=5;
    track[37].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,32);
    track[37].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,35);
    track[37].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,37);
    track[37].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,38);
    track[37].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(6,39);
    trackGraph_copyGraphNodesToReverse(&(track[37].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[37].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C7->18
    track[38].edge[DIR_AHEAD].numGraphNodes=6;
    track[38].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,30);
    track[38].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,29);
    track[38].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,28);
    track[38].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,27);
    track[38].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,26);
    track[38].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,25);
    trackGraph_copyGraphNodesToReverse(&(track[38].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[38].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C8->3
    track[39].edge[DIR_AHEAD].numGraphNodes=7;
    track[39].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,30);
    track[39].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,31);
    track[39].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,32);
    track[39].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,33);
    track[39].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,34);
    track[39].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,35);
    track[39].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,36);
    trackGraph_copyGraphNodesToReverse(&(track[39].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[39].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C9->15
    track[40].edge[DIR_AHEAD].numGraphNodes=5;
    track[40].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,32);
    track[40].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,33);
    track[40].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,34);
    track[40].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,37);
    track[40].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(6,39);
    trackGraph_copyGraphNodesToReverse(&(track[40].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[40].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C10->16
    track[41].edge[DIR_AHEAD].numGraphNodes=6;
    track[41].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,32);
    track[41].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,31);
    track[41].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,30);
    track[41].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,29);
    track[41].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,28);
    track[41].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,27);
    trackGraph_copyGraphNodesToReverse(&(track[41].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[41].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C11->13
    track[42].edge[DIR_AHEAD].numGraphNodes=6;
    track[42].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,32);
    track[42].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,31);
    track[42].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,30);
    track[42].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,29);
    track[42].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,28);
    track[42].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,27);
    trackGraph_copyGraphNodesToReverse(&(track[42].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[42].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C12->14
    track[43].edge[DIR_AHEAD].numGraphNodes=5;
    track[43].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,32);
    track[43].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,33);
    track[43].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,34);
    track[43].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(13,37);
    track[43].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,39);
    trackGraph_copyGraphNodesToReverse(&(track[43].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[43].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C13->E7
    track[44].edge[DIR_AHEAD].numGraphNodes=19;
    track[44].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,29);
    track[44].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,28);
    track[44].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,27);
    track[44].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,26);
    track[44].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,25);
    track[44].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,24);
    track[44].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,23);
    track[44].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,22);
    track[44].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,21);
    track[44].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,20);
    track[44].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,19);
    track[44].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,18);
    track[44].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(16,17);
    track[44].edge[DIR_AHEAD].graphNodes[13]=trackGraph_buildNode(16,16);
    track[44].edge[DIR_AHEAD].graphNodes[14]=trackGraph_buildNode(16,15);
    track[44].edge[DIR_AHEAD].graphNodes[15]=trackGraph_buildNode(16,14);
    track[44].edge[DIR_AHEAD].graphNodes[16]=trackGraph_buildNode(16,13);
    track[44].edge[DIR_AHEAD].graphNodes[17]=trackGraph_buildNode(16,12);
    track[44].edge[DIR_AHEAD].graphNodes[18]=trackGraph_buildNode(16,11);
    trackGraph_copyGraphNodesToReverse(&(track[44].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[44].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C14->11
    track[45].edge[DIR_AHEAD].numGraphNodes=4;
    track[45].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,29);
    track[45].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,30);
    track[45].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,31);
    track[45].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,32);
    trackGraph_copyGraphNodesToReverse(&(track[45].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[45].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C15->D12
    track[46].edge[DIR_AHEAD].numGraphNodes=12;
    track[46].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,25);
    track[46].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,24);
    track[46].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,23);
    track[46].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,22);
    track[46].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,21);
    track[46].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,20);
    track[46].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(2,19);
    track[46].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(2,18);
    track[46].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(2,17);
    track[46].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(2,16);
    track[46].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(2,15);
    track[46].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(2,14);
    trackGraph_copyGraphNodesToReverse(&(track[46].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[46].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C16->6
    track[47].edge[DIR_AHEAD].numGraphNodes=5;
    track[47].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,25);
    track[47].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,26);
    track[47].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,27);
    track[47].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,28);
    track[47].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,29);
    trackGraph_copyGraphNodesToReverse(&(track[47].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[47].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D1->155
    track[48].edge[DIR_AHEAD].numGraphNodes=3;
    track[48].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,18);
    track[48].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,19);
    track[48].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[48].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[48].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D2->E4
    track[49].edge[DIR_AHEAD].numGraphNodes=3;
    track[49].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,18);
    track[49].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,17);
    track[49].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(13,15);
    trackGraph_copyGraphNodesToReverse(&(track[49].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[49].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D3->10
    track[50].edge[DIR_AHEAD].numGraphNodes=4;
    track[50].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,16);
    track[50].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,15);
    track[50].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,14);
    track[50].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,13);
    trackGraph_copyGraphNodesToReverse(&(track[50].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[50].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D5->E6
    track[52].edge[DIR_AHEAD].numGraphNodes=6;
    track[52].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,4);
    track[52].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,6);
    track[52].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,7);
    track[52].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,8);
    track[52].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,9);
    track[52].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,10);
    trackGraph_copyGraphNodesToReverse(&(track[52].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[52].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D6->9
    track[53].edge[DIR_AHEAD].numGraphNodes=3;
    track[53].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,4);
    track[53].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,2);
    track[53].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(11,0);
    trackGraph_copyGraphNodesToReverse(&(track[53].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[53].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D7->9
    track[54].edge[DIR_AHEAD].numGraphNodes=4;
    track[54].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,3);
    track[54].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,2);
    track[54].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,1);
    track[54].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,0);
    trackGraph_copyGraphNodesToReverse(&(track[54].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[54].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D8->E8
    track[55].edge[DIR_AHEAD].numGraphNodes=7;
    track[55].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,3);
    track[55].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,4);
    track[55].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,7);
    track[55].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,8);
    track[55].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,9);
    track[55].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,10);
    track[55].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,11);
    trackGraph_copyGraphNodesToReverse(&(track[55].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[55].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D9->E12
    track[56].edge[DIR_AHEAD].numGraphNodes=3;
    track[56].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,3);
    track[56].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,4);
    track[56].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,7);
    trackGraph_copyGraphNodesToReverse(&(track[56].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[56].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D10->8
    track[57].edge[DIR_AHEAD].numGraphNodes=4;
    track[57].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,3);
    track[57].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(5,2);
    track[57].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(6,1);
    track[57].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,0);
    trackGraph_copyGraphNodesToReverse(&(track[57].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[57].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D12->7
    track[59].edge[DIR_AHEAD].numGraphNodes=5;
    track[59].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,14);
    track[59].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,13);
    track[59].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,12);
    track[59].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,11);
    track[59].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,10);
    trackGraph_copyGraphNodesToReverse(&(track[59].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[59].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D14->17
    track[61].edge[DIR_AHEAD].numGraphNodes=4;
    track[61].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,16);
    track[61].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,15);
    track[61].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,14);
    track[61].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,13);
    trackGraph_copyGraphNodesToReverse(&(track[61].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[61].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D16->17
    track[63].edge[DIR_AHEAD].numGraphNodes=2;
    track[63].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,15);
    track[63].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,13);
    trackGraph_copyGraphNodesToReverse(&(track[63].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[63].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E1->156
    track[64].edge[DIR_AHEAD].numGraphNodes=3;
    track[64].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,22);
    track[64].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,21);
    track[64].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[64].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[64].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E2->15
    track[65].edge[DIR_AHEAD].numGraphNodes=3;
    track[65].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,22);
    track[65].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,23);
    track[65].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(13,25);
    trackGraph_copyGraphNodesToReverse(&(track[65].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[65].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E4->10
    track[67].edge[DIR_AHEAD].numGraphNodes=2;
    track[67].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,15);
    track[67].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,13);
    trackGraph_copyGraphNodesToReverse(&(track[67].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[67].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E6->10
    track[69].edge[DIR_AHEAD].numGraphNodes=4;
    track[69].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10);
    track[69].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,11);
    track[69].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,12);
    track[69].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,13);
    trackGraph_copyGraphNodesToReverse(&(track[69].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[69].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E9->8
    track[72].edge[DIR_AHEAD].numGraphNodes=3;
    track[72].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,4);
    track[72].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,2);
    track[72].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,0);
    trackGraph_copyGraphNodesToReverse(&(track[72].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[72].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E10->E13
    track[73].edge[DIR_AHEAD].numGraphNodes=6;
    track[73].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,4);
    track[73].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,6);
    track[73].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,7);
    track[73].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,8);
    track[73].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,9);
    track[73].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,10);
    trackGraph_copyGraphNodesToReverse(&(track[73].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[73].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E12->7
    track[75].edge[DIR_AHEAD].numGraphNodes=4;
    track[75].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,7);
    track[75].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,8);
    track[75].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,9);
    track[75].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,10);
    trackGraph_copyGraphNodesToReverse(&(track[75].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[75].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E13->17
    track[76].edge[DIR_AHEAD].numGraphNodes=4;
    track[76].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,10);
    track[76].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,11);
    track[76].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,12);
    track[76].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,13);
    trackGraph_copyGraphNodesToReverse(&(track[76].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[76].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E15->13
    track[78].edge[DIR_AHEAD].numGraphNodes=2;
    track[78].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,25);
    track[78].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,27);
    trackGraph_copyGraphNodesToReverse(&(track[78].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[78].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // MR1->2
    track[81].edge[DIR_AHEAD].numGraphNodes=3;
    track[81].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,45);
    track[81].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,43);
    track[81].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[81].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[81].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR2->3
    track[83].edge[DIR_AHEAD].numGraphNodes=3;
    track[83].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,41);
    track[83].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,39);
    track[83].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,36);
    trackGraph_copyGraphNodesToReverse(&(track[83].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[83].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR4->12
    track[87].edge[DIR_AHEAD].numGraphNodes=3;
    track[87].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,44);
    track[87].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,41);
    track[87].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,37);
    trackGraph_copyGraphNodesToReverse(&(track[87].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[87].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR5->18
    track[89].edge[DIR_AHEAD].numGraphNodes=12;
    track[89].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25);
    track[89].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,24);
    track[89].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,23);
    track[89].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,22);
    track[89].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,21);
    track[89].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,20);
    track[89].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,19);
    track[89].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,18);
    track[89].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,17);
    track[89].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,16);
    track[89].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,15);
    track[89].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,14);
    trackGraph_copyGraphNodesToReverse(&(track[89].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[89].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // BR6CURVED->18
    track[90].edge[DIR_CURVED].numGraphNodes=3;
    track[90].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(2,29);
    track[90].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,27);
    track[90].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(0,25);
    trackGraph_copyGraphNodesToReverse(&(track[90].edge[DIR_CURVED]));
    // trackGraph_colorEdge(&(track[90].edge[DIR_CURVED]),whiteColor);
    IOidle(COM2);

    // BR7CURVED->5
    track[92].edge[DIR_CURVED].numGraphNodes=3;
    track[92].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(2,10);
    track[92].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,12);
    track[92].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(0,14);
    trackGraph_copyGraphNodesToReverse(&(track[92].edge[DIR_CURVED]));
    // trackGraph_colorEdge(&(track[92].edge[DIR_CURVED]),whiteColor);
    IOidle(COM2);

    // MR9->MR8
    track[97].edge[DIR_AHEAD].numGraphNodes=5;
    track[97].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,0);
    track[97].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,0);
    track[97].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,0);
    track[97].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,0);
    track[97].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(7,0);
    trackGraph_copyGraphNodesToReverse(&(track[97].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[97].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // BR11CURVEd->14
    track[100].edge[DIR_CURVED].numGraphNodes=5;
    track[100].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(16,32);
    track[100].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,35);
    track[100].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(14,37);
    track[100].edge[DIR_CURVED].graphNodes[3]=trackGraph_buildNode(13,38);
    track[100].edge[DIR_CURVED].graphNodes[4]=trackGraph_buildNode(12,39);
    trackGraph_copyGraphNodesToReverse(&(track[100].edge[DIR_CURVED]));
    // trackGraph_colorEdge(&(track[100].edge[DIR_CURVED]),whiteColor);
    IOidle(COM2);

    // MR12->11
    track[103].edge[DIR_AHEAD].numGraphNodes=8;
    track[103].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,39);
    track[103].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,38);
    track[103].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,37);
    track[103].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,36);
    track[103].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,35);
    track[103].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,34);
    track[103].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,33);
    track[103].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,32);
    trackGraph_copyGraphNodesToReverse(&(track[103].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[103].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR153->154
    track[117].edge[DIR_AHEAD].numGraphNodes=2;
    track[117].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20);
    track[117].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[117].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR154->156
    track[119].edge[DIR_AHEAD].numGraphNodes=2;
    track[119].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20);
    track[119].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[119].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR155->156
    track[121].edge[DIR_AHEAD].numGraphNodes=2;
    track[121].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20);
    track[121].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[121].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR156->154
    track[123].edge[DIR_AHEAD].numGraphNodes=2;
    track[123].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20);
    track[123].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[123].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // EN1->155
    track[124].edge[DIR_AHEAD].numGraphNodes=4;
    track[124].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(6,20);
    track[124].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,20);
    track[124].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(8,20);
    track[124].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[124].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[126].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // EN2->153
    track[126].edge[DIR_AHEAD].numGraphNodes=4;
    track[126].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,20);
    track[126].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(11,20);
    track[126].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,20);
    track[126].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,20);
    trackGraph_copyGraphNodesToReverse(&(track[126].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[124].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);
}

void trackGraph_extractNodeInfo(unsigned int graphNodeInfo, int *row, int *col) {
    *row = graphNodeInfo % 100;
    *col = graphNodeInfo / 100;
}

unsigned int trackGraph_buildNode(int row, int col) {
    assert(row >= 0 && col >= 0 && row < 100 && col < 100, "trackGraph_buildNode: invalid location.");
    return row + col * 100;
}

void trackGraph_copyGraphNodesToReverse(track_edge *edge) {
    track_edge *reverse = edge->reverse;
    int numNodes = edge->numGraphNodes;
    reverse->numGraphNodes = numNodes;

    int i = 0;
    for (; i < numNodes; i++) {
        reverse->graphNodes[numNodes -1 - i] = edge->graphNodes[i];
    }
}
