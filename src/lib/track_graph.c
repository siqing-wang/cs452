#include <track_graph.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <trainset.h>

#define SW_HIGHLIGHT_COLOR 32
#define PATH_HIGHLIGHT_COLOR 31
#define UNHIGHLIGHT_COLOR 37


/* Have each graph node's info inside a signle int to save space. */
void trackGraph_extractNodeInfo(unsigned int graphNodeInfo, int *row, int *col, int *isSwPath);
unsigned int trackGraph_buildNode(int row, int col, int isSwPath);

/* Initialization Helper. */
void trackGraph_copyGraphNodesToReverse(track_edge *edge);

/* Drawing helper. */
void trackGraph_colorEdge(track_edge *edge, int color);
void trackGraph_colorSwitch(track_edge *edge, int color);
void trackGraph_colorTillNextSensor(struct TrainSetData *data, track_node *node, int color);

void trackGraph_colorEdge(track_edge *edge, int color) {
    int row, col, isSwPath;

    int i;
    PrintfAt(COM2, 0, 0, "\033[%dm", color);
    for (i = 0; i < edge->numGraphNodes; i++) {
        trackGraph_extractNodeInfo(edge->graphNodes[i], &row, &col, &isSwPath);
        PrintfAt(COM2, TRACK_R+row, TRACK_C+col, "*");
        IOidle(COM2);
    }
    PutStr(COM2, TCS_RESET);
}

/* Switch is represented by 3 dots, determine if the forward one is still used by some other switch,
 * i.e. cannot unhighlight it */
int sharedForwardSwPath(track_edge *edge) {

    track_node *dest = edge->dest;
    if (dest->type != NODE_BRANCH && dest->type != NODE_MERGE) {
        return 0;
    }

    int row, col, isSwPath;
    int rowDestRev, colDestRev, isSwPathDestRev;
    trackGraph_extractNodeInfo(edge->graphNodes[1], &row, &col, &isSwPath);

    track_node *destRev = dest->reverse;
    if (dest->type == NODE_MERGE) {
        // destRev is a branch
        trackGraph_extractNodeInfo(destRev->edge[0].graphNodes[1], &rowDestRev, &colDestRev, &isSwPathDestRev);
        if (row == rowDestRev && col == colDestRev) {
            return isSwPathDestRev;
        }
        trackGraph_extractNodeInfo(destRev->edge[1].graphNodes[1], &rowDestRev, &colDestRev, &isSwPathDestRev);
        if (row == rowDestRev && col == colDestRev) {
            return isSwPathDestRev;
        }
    } else if (dest->type == NODE_BRANCH) {
        // destRev is a merge
        trackGraph_extractNodeInfo(destRev->edge[0].graphNodes[1], &rowDestRev, &colDestRev, &isSwPathDestRev);
        if (row == rowDestRev && col == colDestRev) {
            return isSwPathDestRev;
        }
    }
    return 0;
}

/* Switch is represented by 3 dots, determine if the backward one is still used by some other switch,
 * i.e. cannot unhighlight it */
int sharedBackwardSwPath(track_edge *edge) {

    track_node *dest = edge->src->reverse->edge[0].dest;
    if (dest->type != NODE_BRANCH && dest->type != NODE_MERGE) {
        return 0;
    }

    int row, col, isSwPath;
    int rowDestRev, colDestRev, isSwPathDestRev;
    track_edge *oppositeEdge = &(edge->src->reverse->edge[0]);
    trackGraph_extractNodeInfo(oppositeEdge->graphNodes[1], &row, &col, &isSwPath);

    track_node *destRev = dest->reverse;
    if (dest->type == NODE_MERGE) {
        // destRev is a branch
        trackGraph_extractNodeInfo(destRev->edge[0].graphNodes[1], &rowDestRev, &colDestRev, &isSwPathDestRev);
        if (row == rowDestRev && col == colDestRev) {
            return isSwPathDestRev;
        }
        trackGraph_extractNodeInfo(destRev->edge[1].graphNodes[1], &rowDestRev, &colDestRev, &isSwPathDestRev);
        if (row == rowDestRev && col == colDestRev) {
            return isSwPathDestRev;
        }
    } else if (dest->type == NODE_BRANCH) {
        // destRev is a merge
        trackGraph_extractNodeInfo(destRev->edge[0].graphNodes[1], &rowDestRev, &colDestRev, &isSwPathDestRev);
        if (row == rowDestRev && col == colDestRev) {
            return isSwPathDestRev;
        }
    }
    return 0;
}

void trackGraph_colorSwitch(track_edge *edge, int color) {

    assert(edge->src->type == NODE_BRANCH, "trackGraph_colorSwitch: not a switch");

    PrintfAt(COM2, 0, 0, "\033[%dm", color);

    int row, col, isSwPath;
    int newIsSwPath = color == UNHIGHLIGHT_COLOR ? 0 : 1;

    trackGraph_extractNodeInfo(edge->graphNodes[0], &row, &col, &isSwPath);
    edge->graphNodes[0] = trackGraph_buildNode(row, col, newIsSwPath);
    PrintfAt(COM2, TRACK_R+row, TRACK_C+col, "*");

    trackGraph_extractNodeInfo(edge->graphNodes[1], &row, &col, &isSwPath);
    edge->graphNodes[1] = trackGraph_buildNode(row, col, newIsSwPath);
    if (color != UNHIGHLIGHT_COLOR || !sharedForwardSwPath(edge)) {
        PrintfAt(COM2, TRACK_R+row, TRACK_C+col, "*");
    }

    track_edge *oppositeEdge = &(edge->src->reverse->edge[0]);
    trackGraph_extractNodeInfo(oppositeEdge->graphNodes[1], &row, &col, &isSwPath);
    oppositeEdge->graphNodes[1] = trackGraph_buildNode(row, col, newIsSwPath);
    if (color != UNHIGHLIGHT_COLOR || !sharedBackwardSwPath(edge)) {
        PrintfAt(COM2, TRACK_R+row, TRACK_C+col, "*");
    }
    PutStr(COM2, TCS_RESET);
    IOidle(COM2);
}

void trackGraph_colorTillNextSensor(struct TrainSetData *data, track_node *node, int color) {

    int *swtable = data->swtable;
    int corresSwNo, direction;
    track_node *startNode = node;
    for (;;) {
        /* Color until reaching another sensor. */
        if (node->type == NODE_EXIT) {
            return;
        } else if (node->type == NODE_SENSOR && node != startNode) {
            /* Reached the next node, stop coloring. */
            return;
        }

        direction = 0;

        if (node->type == NODE_BRANCH) {
            /* Get branch's current direction from switch table. */
            corresSwNo = node->num;
            direction = *(swtable + getSwitchIndex(corresSwNo));
        }

        trackGraph_colorEdge(&(node->edge[direction]), color);
        if (color == UNHIGHLIGHT_COLOR) {
            if(node->type == NODE_BRANCH) {
                trackGraph_colorSwitch(&(node->edge[direction]), SW_HIGHLIGHT_COLOR);
            } else if (node->type == NODE_MERGE) {
                track_node *brrev = node->reverse;
                int brrevDir = *(swtable + getSwitchIndex(brrev->num));
                trackGraph_colorSwitch(&(brrev->edge[brrevDir]), SW_HIGHLIGHT_COLOR);
            }
        }

        node = node->edge[direction].dest;
    }
}

void trackGraph_highlightSenPath(struct TrainSetData *data, track_node *node) {
    trackGraph_colorTillNextSensor(data, node, PATH_HIGHLIGHT_COLOR);
}

void trackGraph_unhighlightSenPath(struct TrainSetData *data, track_node *node) {
    trackGraph_colorTillNextSensor(data, node, UNHIGHLIGHT_COLOR);
}

void trackGraph_turnSw(struct TrainSetData *data, int switchNumber, int newDir) {
    int indexInTrack = getSwitchIndex(switchNumber) * 2 + 80;
    track_node *node = data->track + indexInTrack;
    trackGraph_colorSwitch(&(node->edge[1 - newDir]), UNHIGHLIGHT_COLOR);
    trackGraph_colorSwitch(&(node->edge[newDir]), SW_HIGHLIGHT_COLOR);
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
    track[0].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(0,11,0);
    track[0].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(0,12,0);
    track[0].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(0,13,0);
    track[0].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(0,14,0);
    track[0].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(0,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[0].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[0].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A2 -> EX5
    track[1].edge[DIR_AHEAD].numGraphNodes=12;
    track[1].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,11,0);
    track[1].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,10,0);
    track[1].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,9,0);
    track[1].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,8,0);
    track[1].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,7,0);
    track[1].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,6,0);
    track[1].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,5,0);
    track[1].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,4,0);
    track[1].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,3,0);
    track[1].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,2,0);
    track[1].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,1,0);
    track[1].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[1].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[1].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A3 -> 14
    track[2].edge[DIR_AHEAD].numGraphNodes=2;
    track[2].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,14,0);
    track[2].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[2].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[2].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A4 -> B16
    track[3].edge[DIR_AHEAD].numGraphNodes=5;
    track[3].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,14,0);
    track[3].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,14,0);
    track[3].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,14,0);
    track[3].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,14,0);
    track[3].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,14,0);
    trackGraph_copyGraphNodesToReverse(&(track[3].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[3].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A5 -> 3   to 16
    track[4].edge[DIR_AHEAD].numGraphNodes=6;
    track[4].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,13,0);
    track[4].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,14,0);
    track[4].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,15,0);
    track[4].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,16,0);
    track[4].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,17,0);
    track[4].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,18,0);
    trackGraph_copyGraphNodesToReverse(&(track[4].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[4].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A6 -> B10
    track[5].edge[DIR_AHEAD].numGraphNodes=13;
    track[5].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,13,0);
    track[5].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,12,0);
    track[5].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,11,0);
    track[5].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,10,0);
    track[5].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,9,0);
    track[5].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,8,0);
    track[5].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,7,0);
    track[5].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,6,0);
    track[5].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,5,0);
    track[5].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,4,0);
    track[5].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,3,0);
    track[5].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,2,0);
    track[5].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(16,1,0);
    trackGraph_copyGraphNodesToReverse(&(track[5].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[5].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A7 -> B12
    track[6].edge[DIR_AHEAD].numGraphNodes=10;
    track[6].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10,0);
    track[6].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,9,0);
    track[6].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,8,0);
    track[6].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,7,0);
    track[6].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,6,0);
    track[6].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,5,0);
    track[6].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,4,0);
    track[6].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,3,0);
    track[6].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(14,2,0);
    track[6].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(14,1,0);
    trackGraph_copyGraphNodesToReverse(&(track[6].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[6].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A8 -> 2
    track[7].edge[DIR_AHEAD].numGraphNodes=4;
    track[7].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10,0);
    track[7].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,11,0);
    track[7].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,12,0);
    track[7].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[7].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[7].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A9 -> B8
    track[8].edge[DIR_AHEAD].numGraphNodes=6;
    track[8].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,7,0);
    track[8].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,6,0);
    track[8].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,5,0);
    track[8].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,4,0);
    track[8].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,3,0);
    track[8].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,2,0);
    trackGraph_copyGraphNodesToReverse(&(track[8].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[8].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A10 -> 1
    track[9].edge[DIR_AHEAD].numGraphNodes=4;
    track[9].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,7,0);
    track[9].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,8,0);
    track[9].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,9,0);
    track[9].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,10,0);
    trackGraph_copyGraphNodesToReverse(&(track[9].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[9].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A11 -> 1
    track[10].edge[DIR_AHEAD].numGraphNodes=9;
    track[10].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,2,0);
    track[10].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,3,0);
    track[10].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,4,0);
    track[10].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(10,5,0);
    track[10].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,6,0);
    track[10].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(10,7,0);
    track[10].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(10,8,0);
    track[10].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(11,9,0);
    track[10].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(12,10,0);
    trackGraph_copyGraphNodesToReverse(&(track[10].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[10].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A12 -> EX
    track[11].edge[DIR_AHEAD].numGraphNodes=2;
    track[11].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,2,0);
    track[11].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,1,0);
    trackGraph_copyGraphNodesToReverse(&(track[11].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[11].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A13 -> 4
    track[12].edge[DIR_AHEAD].numGraphNodes = 4;
    track[12].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(2, 8,0);
    track[12].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(2, 9,0);
    track[12].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(2, 10,0);
    track[12].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(2, 11,0);
    trackGraph_copyGraphNodesToReverse(&(track[12].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[12].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A14 -> EX
    track[13].edge[DIR_AHEAD].numGraphNodes = 9;
    track[13].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(2, 8,0);
    track[13].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(2, 7,0);
    track[13].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(2, 6,0);
    track[13].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(2, 5,0);
    track[13].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(2, 4,0);
    track[13].edge[DIR_AHEAD].graphNodes[5] = trackGraph_buildNode(2, 3,0);
    track[13].edge[DIR_AHEAD].graphNodes[6] = trackGraph_buildNode(2, 2,0);
    track[13].edge[DIR_AHEAD].graphNodes[7] = trackGraph_buildNode(2, 1,0);
    track[13].edge[DIR_AHEAD].graphNodes[8] = trackGraph_buildNode(2, 0,0);
    trackGraph_copyGraphNodesToReverse(&(track[13].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[13].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A15 -> EX
    track[14].edge[DIR_AHEAD].numGraphNodes = 5;
    track[14].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(4, 5,0);
    track[14].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(4, 4,0);
    track[14].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(4, 3,0);
    track[14].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(4, 2,0);
    track[14].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(4, 1,0);
    trackGraph_copyGraphNodesToReverse(&(track[14].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[14].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A16 -> 4
    track[15].edge[DIR_AHEAD].numGraphNodes=6;
    track[15].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,5,0);
    track[15].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,6,0);
    track[15].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,7,0);
    track[15].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,8,0);
    track[15].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(3,10,0);
    track[15].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,11,0);
    trackGraph_copyGraphNodesToReverse(&(track[15].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[15].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B1 -> D14
    track[16].edge[DIR_AHEAD].numGraphNodes=9;
    track[16].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,30,0);
    track[16].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,31,0);
    track[16].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,32,0);
    track[16].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,33,0);
    track[16].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,34,0);
    track[16].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,35,0);
    track[16].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(12,36,0);
    track[16].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(12,37,0);
    track[16].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(12,38,0);
    trackGraph_copyGraphNodesToReverse(&(track[16].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[16].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B2 -> 16
    track[17].edge[DIR_AHEAD].numGraphNodes=4;
    track[17].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,30,0);
    track[17].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,29,0);
    track[17].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,28,0);
    track[17].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[17].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[17].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B3 -> C2
    track[18].edge[DIR_AHEAD].numGraphNodes=3;
    track[18].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,29,0);
    track[18].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,31,0);
    track[18].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,32,0);
    trackGraph_copyGraphNodesToReverse(&(track[18].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[18].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B4 -> 16
    track[19].edge[DIR_AHEAD].numGraphNodes=2;
    track[19].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,29,0);
    track[19].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[19].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[19].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B5 -> D3
    track[20].edge[DIR_AHEAD].numGraphNodes=9;
    track[20].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,30,0);
    track[20].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,31,0);
    track[20].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,32,0);
    track[20].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,33,0);
    track[20].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,34,0);
    track[20].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,35,0);
    track[20].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(2,36,0);
    track[20].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(2,37,0);
    track[20].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(2,38,0);
    trackGraph_copyGraphNodesToReverse(&(track[20].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[20].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B6 -> 13
    track[21].edge[DIR_AHEAD].numGraphNodes=4;
    track[21].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,30,0);
    track[21].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,29,0);
    track[21].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,28,0);
    track[21].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[21].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[21].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B8 -> EX
    track[23].edge[DIR_AHEAD].numGraphNodes=2;
    track[23].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,2,0);
    track[23].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,1,0);
    trackGraph_copyGraphNodesToReverse(&(track[23].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[23].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B10 -> EX
    track[25].edge[DIR_AHEAD].numGraphNodes=2;
    track[25].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,1,0);
    track[25].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[25].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[25].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B12 -> EX
    track[27].edge[DIR_AHEAD].numGraphNodes=2;
    track[27].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,1,0);
    track[27].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[27].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[27].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B13 -> 154
    track[28].edge[DIR_AHEAD].numGraphNodes=3;
    track[28].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,36,0);
    track[28].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,35,0);
    track[28].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[28].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[28].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B14 -> D16
    track[29].edge[DIR_AHEAD].numGraphNodes=3;
    track[29].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,36,0);
    track[29].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,37,0);
    track[29].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(11,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[29].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[29].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B16 -> 15
    track[31].edge[DIR_AHEAD].numGraphNodes=2;
    track[31].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,14,0);
    track[31].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[31].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[31].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C2 -> 153
    track[33].edge[DIR_AHEAD].numGraphNodes=3;
    track[33].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,32,0);
    track[33].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,33,0);
    track[33].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[33].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[33].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C3 -> EX3
    track[34].edge[DIR_AHEAD].numGraphNodes=9;
    track[34].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,45,0);
    track[34].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,46,0);
    track[34].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,47,0);
    track[34].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,48,0);
    track[34].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,49,0);
    track[34].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,50,0);
    track[34].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,51,0);
    track[34].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,52,0);
    track[34].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,53,0);
    trackGraph_copyGraphNodesToReverse(&(track[34].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[34].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C4 -> 5
    track[35].edge[DIR_AHEAD].numGraphNodes=6;
    track[35].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,45,0);
    track[35].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,44,0);
    track[35].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,43,0);
    track[35].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,42,0);
    track[35].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,41,0);
    track[35].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,40,0);
    trackGraph_copyGraphNodesToReverse(&(track[35].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[35].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C5 -> 6
    track[36].edge[DIR_AHEAD].numGraphNodes=4;
    track[36].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,22,0);
    track[36].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,23,0);
    track[36].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,24,0);
    track[36].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,25,0);
    trackGraph_copyGraphNodesToReverse(&(track[36].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[36].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C6 -> 15
    track[37].edge[DIR_AHEAD].numGraphNodes=5;
    track[37].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,22,0);
    track[37].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,19,0);
    track[37].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,17,0);
    track[37].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,16,0);
    track[37].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[37].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[37].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C7 -> 18
    track[38].edge[DIR_AHEAD].numGraphNodes=6;
    track[38].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,24,0);
    track[38].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,25,0);
    track[38].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,26,0);
    track[38].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,27,0);
    track[38].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,28,0);
    track[38].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,29,0);
    trackGraph_copyGraphNodesToReverse(&(track[38].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[38].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C8 -> 3
    track[39].edge[DIR_AHEAD].numGraphNodes=7;
    track[39].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,24,0);
    track[39].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,23,0);
    track[39].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,22,0);
    track[39].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,21,0);
    track[39].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,20,0);
    track[39].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,19,0);
    track[39].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,18,0);
    trackGraph_copyGraphNodesToReverse(&(track[39].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[39].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C9 -> 15
    track[40].edge[DIR_AHEAD].numGraphNodes=5;
    track[40].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,22,0);
    track[40].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,21,0);
    track[40].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,20,0);
    track[40].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,17,0);
    track[40].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[40].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[40].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C10 -> 16
    track[41].edge[DIR_AHEAD].numGraphNodes=6;
    track[41].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,22,0);
    track[41].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,23,0);
    track[41].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,24,0);
    track[41].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,25,0);
    track[41].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,26,0);
    track[41].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[41].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[41].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C11 -> 13
    track[42].edge[DIR_AHEAD].numGraphNodes=6;
    track[42].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,22,0);
    track[42].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,23,0);
    track[42].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,24,0);
    track[42].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,25,0);
    track[42].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,26,0);
    track[42].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[42].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[42].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C12 -> 14
    track[43].edge[DIR_AHEAD].numGraphNodes=5;
    track[43].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,22,0);
    track[43].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,21,0);
    track[43].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,20,0);
    track[43].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(3,17,0);
    track[43].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[43].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[43].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);


    // C13 -> E7
    track[44].edge[DIR_AHEAD].numGraphNodes=19;
    track[44].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25,0);
    track[44].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,26,0);
    track[44].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,27,0);
    track[44].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,28,0);
    track[44].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,29,0);
    track[44].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,30,0);
    track[44].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,31,0);
    track[44].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,32,0);
    track[44].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,33,0);
    track[44].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,34,0);
    track[44].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,35,0);
    track[44].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,36,0);
    track[44].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(0,37,0);
    track[44].edge[DIR_AHEAD].graphNodes[13]=trackGraph_buildNode(0,38,0);
    track[44].edge[DIR_AHEAD].graphNodes[14]=trackGraph_buildNode(0,39,0);
    track[44].edge[DIR_AHEAD].graphNodes[15]=trackGraph_buildNode(0,40,0);
    track[44].edge[DIR_AHEAD].graphNodes[16]=trackGraph_buildNode(0,41,0);
    track[44].edge[DIR_AHEAD].graphNodes[17]=trackGraph_buildNode(0,42,0);
    track[44].edge[DIR_AHEAD].graphNodes[18]=trackGraph_buildNode(0,43,0);
    trackGraph_copyGraphNodesToReverse(&(track[44].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[44].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C14 -> 11
    track[45].edge[DIR_AHEAD].numGraphNodes=4;
    track[45].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25,0);
    track[45].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,24,0);
    track[45].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,23,0);
    track[45].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,22,0);
    trackGraph_copyGraphNodesToReverse(&(track[45].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[45].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C15 -> D12
    track[46].edge[DIR_AHEAD].numGraphNodes=12;
    track[46].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,29,0);
    track[46].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,30,0);
    track[46].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,31,0);
    track[46].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,32,0);
    track[46].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,33,0);
    track[46].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,34,0);
    track[46].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,35,0);
    track[46].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,36,0);
    track[46].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(14,37,0);
    track[46].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(14,38,0);
    track[46].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(14,39,0);
    track[46].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(14,40,0);
    trackGraph_copyGraphNodesToReverse(&(track[46].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[46].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C16 -> 6
    track[47].edge[DIR_AHEAD].numGraphNodes=5;
    track[47].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,29,0);
    track[47].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,28,0);
    track[47].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,27,0);
    track[47].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,26,0);
    track[47].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,25,0);
    trackGraph_copyGraphNodesToReverse(&(track[47].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[47].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D1 -> 155
    track[48].edge[DIR_AHEAD].numGraphNodes=3;
    track[48].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,36,0);
    track[48].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,35,0);
    track[48].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[48].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[48].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D2 -> E4
    track[49].edge[DIR_AHEAD].numGraphNodes=3;
    track[49].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,36,0);
    track[49].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,37,0);
    track[49].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(3,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[49].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[49].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D3 -> 10
    track[50].edge[DIR_AHEAD].numGraphNodes=4;
    track[50].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,38,0);
    track[50].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,39,0);
    track[50].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,40,0);
    track[50].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[50].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[50].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D5 -> E6
    track[52].edge[DIR_AHEAD].numGraphNodes=6;
    track[52].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,50,0);
    track[52].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,48,0);
    track[52].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,47,0);
    track[52].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,46,0);
    track[52].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,45,0);
    track[52].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,44,0);
    trackGraph_copyGraphNodesToReverse(&(track[52].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[52].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D6 -> 9
    track[53].edge[DIR_AHEAD].numGraphNodes=3;
    track[53].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,50,0);
    track[53].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,52,0);
    track[53].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(5,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[53].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[53].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D7 -> 9
    track[54].edge[DIR_AHEAD].numGraphNodes=4;
    track[54].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,51,0);
    track[54].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,52,0);
    track[54].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,53,0);
    track[54].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[54].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[54].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D8 -> E8
    track[55].edge[DIR_AHEAD].numGraphNodes=7;
    track[55].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,51,0);
    track[55].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,50,0);
    track[55].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,47,0);
    track[55].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,46,0);
    track[55].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,45,0);
    track[55].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,44,0);
    track[55].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,43,0);
    trackGraph_copyGraphNodesToReverse(&(track[55].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[55].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D9 -> E12
    track[56].edge[DIR_AHEAD].numGraphNodes=3;
    track[56].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,51,0);
    track[56].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,50,0);
    track[56].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,47,0);
    trackGraph_copyGraphNodesToReverse(&(track[56].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[56].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D10 -> 8
    track[57].edge[DIR_AHEAD].numGraphNodes=4;
    track[57].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,51,0);
    track[57].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(11,52,0);
    track[57].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,53,0);
    track[57].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[57].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[57].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D12 -> 7
    track[59].edge[DIR_AHEAD].numGraphNodes=5;
    track[59].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,40,0);
    track[59].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,41,0);
    track[59].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,42,0);
    track[59].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,43,0);
    track[59].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,44,0);
    trackGraph_copyGraphNodesToReverse(&(track[59].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[59].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D14 -> 17
    track[61].edge[DIR_AHEAD].numGraphNodes=4;
    track[61].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,38,0);
    track[61].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,39,0);
    track[61].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,40,0);
    track[61].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[61].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[61].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D16 -> 17
    track[63].edge[DIR_AHEAD].numGraphNodes=2;
    track[63].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,39,0);
    track[63].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[63].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[63].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E1 -> 156
    track[64].edge[DIR_AHEAD].numGraphNodes=3;
    track[64].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,32,0);
    track[64].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,33,0);
    track[64].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[64].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[64].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E2 -> 15
    track[65].edge[DIR_AHEAD].numGraphNodes=3;
    track[65].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,32,0);
    track[65].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,31,0);
    track[65].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(3,29,0);
    trackGraph_copyGraphNodesToReverse(&(track[65].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[65].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E4 -> 10
    track[67].edge[DIR_AHEAD].numGraphNodes=2;
    track[67].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,39,0);
    track[67].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[67].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[67].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E6 -> 10
    track[69].edge[DIR_AHEAD].numGraphNodes=4;
    track[69].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44,0);
    track[69].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,43,0);
    track[69].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,42,0);
    track[69].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[69].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[69].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E9 -> 8
    track[72].edge[DIR_AHEAD].numGraphNodes=3;
    track[72].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,50,0);
    track[72].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,52,0);
    track[72].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[72].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[72].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E10 -> E13
    track[73].edge[DIR_AHEAD].numGraphNodes=6;
    track[73].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,50,0);
    track[73].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,48,0);
    track[73].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,47,0);
    track[73].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,46,0);
    track[73].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,45,0);
    track[73].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(12,44,0);
    trackGraph_copyGraphNodesToReverse(&(track[73].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[73].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E12 -> 7
    track[75].edge[DIR_AHEAD].numGraphNodes=4;
    track[75].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,47,0);
    track[75].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,46,0);
    track[75].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,45,0);
    track[75].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,44,0);
    trackGraph_copyGraphNodesToReverse(&(track[75].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[75].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E13 -> 17
    track[76].edge[DIR_AHEAD].numGraphNodes=4;
    track[76].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,44,0);
    track[76].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,43,0);
    track[76].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,42,0);
    track[76].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[76].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[76].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E15 -> 13
    track[78].edge[DIR_AHEAD].numGraphNodes=2;
    track[78].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,29,0);
    track[78].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[78].edge[DIR_AHEAD]));
    //trackGraph_colorEdge(&(track[78].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // MR1 -> 2
    track[81].edge[DIR_AHEAD].numGraphNodes=3;
    track[81].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,10,0);
    track[81].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,11,0);
    track[81].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[81].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[81].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR2 -> 3
    track[83].edge[DIR_AHEAD].numGraphNodes=3;
    track[83].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,13,0);
    track[83].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,15,0);
    track[83].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,18,0);
    trackGraph_copyGraphNodesToReverse(&(track[83].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[83].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR4 -> 12
    track[87].edge[DIR_AHEAD].numGraphNodes=3;
    track[87].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,11,0);
    track[87].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,13,0);
    track[87].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[87].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[87].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR5 -> 18
    track[89].edge[DIR_AHEAD].numGraphNodes=12;
    track[89].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,29,0);
    track[89].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,30,0);
    track[89].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,31,0);
    track[89].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,32,0);
    track[89].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,33,0);
    track[89].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,34,0);
    track[89].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,35,0);
    track[89].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,36,0);
    track[89].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,37,0);
    track[89].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,38,0);
    track[89].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,39,0);
    track[89].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,40,0);
    trackGraph_copyGraphNodesToReverse(&(track[89].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[89].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // BR6 CURVED-> 18
    track[90].edge[DIR_CURVED].numGraphNodes=3;
    track[90].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(14,25,0);
    track[90].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,27,0);
    track[90].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(16,29,0);
    trackGraph_copyGraphNodesToReverse(&(track[90].edge[DIR_CURVED]));
    //trackGraph_colorEdge(&(track[90].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // BR7 CURVED-> 5
    track[92].edge[DIR_CURVED].numGraphNodes=3;
    track[92].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(14,44,0);
    track[92].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,42,0);
    track[92].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(16,40,0);
    trackGraph_copyGraphNodesToReverse(&(track[92].edge[DIR_CURVED]));
    //trackGraph_colorEdge(&(track[92].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // MR9 -> MR8
    track[97].edge[DIR_AHEAD].numGraphNodes=5;
    track[97].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,54,0);
    track[97].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,54,0);
    track[97].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,54,0);
    track[97].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,54,0);
    track[97].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[97].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[97].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // BR11 CURVEd-> 14
    track[100].edge[DIR_CURVED].numGraphNodes=5;
    track[100].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(0,22,0);
    track[100].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,19,0);
    track[100].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(2,17,0);
    track[100].edge[DIR_CURVED].graphNodes[3]=trackGraph_buildNode(3,16,0);
    track[100].edge[DIR_CURVED].graphNodes[4]=trackGraph_buildNode(4,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[100].edge[DIR_CURVED]));
    //trackGraph_colorEdge(&(track[100].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // MR12 -> 11
    track[103].edge[DIR_AHEAD].numGraphNodes=8;
    track[103].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,15,0);
    track[103].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,16,0);
    track[103].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,17,0);
    track[103].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,18,0);
    track[103].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,19,0);
    track[103].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,20,0);
    track[103].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,21,0);
    track[103].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,22,0);
    trackGraph_copyGraphNodesToReverse(&(track[103].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[103].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR153 -> 154
    track[117].edge[DIR_AHEAD].numGraphNodes=2;
    track[117].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34,0);
    track[117].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[117].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR154 -> 156
    track[119].edge[DIR_AHEAD].numGraphNodes=2;
    track[119].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34,0);
    track[119].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[119].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR155 -> 156
    track[121].edge[DIR_AHEAD].numGraphNodes=2;
    track[121].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34,0);
    track[121].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[121].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR156 -> 154
    track[123].edge[DIR_AHEAD].numGraphNodes=2;
    track[123].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34,0);
    track[123].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[123].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // EN1 -> 155
    track[124].edge[DIR_AHEAD].numGraphNodes=4;
    track[124].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,34,0);
    track[124].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,34,0);
    track[124].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(8,34,0);
    track[124].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[124].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[126].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // EN2 -> 153
    track[126].edge[DIR_AHEAD].numGraphNodes=4;
    track[126].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,34,0);
    track[126].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(5,34,0);
    track[126].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(6,34,0);
    track[126].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,34,0);
    trackGraph_copyGraphNodesToReverse(&(track[126].edge[DIR_STRAIGHT]));
    //trackGraph_colorEdge(&(track[124].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    /* Initialize switch in graph. */
    int i = 0;
    for ( ; i < SWITCH_TOTAL; i++) {
        trackGraph_colorSwitch(&(track[i * 2 + 80].edge[data->swtable[i]]), SW_HIGHLIGHT_COLOR);
    }
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
    track[0].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,40,0);
    track[0].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,39,0);
    track[0].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,38,0);
    track[0].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,37,0);
    trackGraph_copyGraphNodesToReverse(&(track[0].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[0].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A2->EX5
    track[1].edge[DIR_AHEAD].numGraphNodes=12;
    track[1].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,43,0);
    track[1].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,44,0);
    track[1].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,45,0);
    track[1].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,46,0);
    track[1].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,47,0);
    track[1].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,48,0);
    track[1].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,49,0);
    track[1].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,50,0);
    track[1].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,51,0);
    track[1].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,52,0);
    track[1].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,53,0);
    track[1].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[1].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[1].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A3->14
    track[2].edge[DIR_AHEAD].numGraphNodes=2;
    track[2].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,40,0);
    track[2].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[2].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[2].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A4->B16
    track[3].edge[DIR_AHEAD].numGraphNodes=5;
    track[3].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,40,0);
    track[3].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,40,0);
    track[3].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,40,0);
    track[3].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,40,0);
    track[3].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(7,40,0);
    trackGraph_copyGraphNodesToReverse(&(track[3].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[3].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A5->3to16
    track[4].edge[DIR_AHEAD].numGraphNodes=6;
    track[4].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,41,0);
    track[4].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,40,0);
    track[4].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,39,0);
    track[4].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,38,0);
    track[4].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,37,0);
    track[4].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,36,0);
    trackGraph_copyGraphNodesToReverse(&(track[4].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[4].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A6->B10
    track[5].edge[DIR_AHEAD].numGraphNodes=13;
    track[5].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,41,0);
    track[5].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,42,0);
    track[5].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,43,0);
    track[5].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,44,0);
    track[5].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,45,0);
    track[5].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,46,0);
    track[5].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,47,0);
    track[5].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,48,0);
    track[5].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,49,0);
    track[5].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,50,0);
    track[5].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,51,0);
    track[5].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,52,0);
    track[5].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(0,53,0);
    trackGraph_copyGraphNodesToReverse(&(track[5].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[5].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A7->B12
    track[6].edge[DIR_AHEAD].numGraphNodes=10;
    track[6].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44,0);
    track[6].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,45,0);
    track[6].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,46,0);
    track[6].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,47,0);
    track[6].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,48,0);
    track[6].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,49,0);
    track[6].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(2,50,0);
    track[6].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(2,51,0);
    track[6].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(2,52,0);
    track[6].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(2,53,0);
    trackGraph_copyGraphNodesToReverse(&(track[6].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[6].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A8->2
    track[7].edge[DIR_AHEAD].numGraphNodes=4;
    track[7].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44,0);
    track[7].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,43,0);
    track[7].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,42,0);
    track[7].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[7].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[7].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A9->B8
    track[8].edge[DIR_AHEAD].numGraphNodes=6;
    track[8].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,47,0);
    track[8].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,48,0);
    track[8].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,49,0);
    track[8].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,50,0);
    track[8].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,51,0);
    track[8].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,52,0);
    trackGraph_copyGraphNodesToReverse(&(track[8].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[8].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A10->1
    track[9].edge[DIR_AHEAD].numGraphNodes=3;
    track[9].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,47,0);
    track[9].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,46,0);
    track[9].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,45,0);
    trackGraph_copyGraphNodesToReverse(&(track[9].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[9].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A11->1
    track[10].edge[DIR_AHEAD].numGraphNodes=2;
    track[10].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,47,0);
    track[10].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,45,0);
    trackGraph_copyGraphNodesToReverse(&(track[10].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[10].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A12->A16
    track[11].edge[DIR_AHEAD].numGraphNodes=9;
    track[11].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,47,0);
    track[11].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,49,0);
    track[11].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,50,0);
    track[11].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,50,0);
    track[11].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,50,0);
    track[11].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(10,50,0);
    track[11].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(11,50,0);
    track[11].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(12,49,0);
    track[11].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(13,47,0);
    trackGraph_copyGraphNodesToReverse(&(track[11].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[11].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A13->4
    track[12].edge[DIR_AHEAD].numGraphNodes=3;
    track[12].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,46,0);
    track[12].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,45,0);
    track[12].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,44,0);
    trackGraph_copyGraphNodesToReverse(&(track[12].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[12].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A14->EX
    track[13].edge[DIR_AHEAD].numGraphNodes=8;
    track[13].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,46,0);
    track[13].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,47,0);
    track[13].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,48,0);
    track[13].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,49,0);
    track[13].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,50,0);
    track[13].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,51,0);
    track[13].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,52,0);
    track[13].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,53,0);
    trackGraph_copyGraphNodesToReverse(&(track[13].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[13].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // A16->4
    track[15].edge[DIR_AHEAD].numGraphNodes=2;
    track[15].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,47,0);
    track[15].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,44,0);
    trackGraph_copyGraphNodesToReverse(&(track[15].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[15].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B1->D14
    track[16].edge[DIR_AHEAD].numGraphNodes=9;
    track[16].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,24,0);
    track[16].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,23,0);
    track[16].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,22,0);
    track[16].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,21,0);
    track[16].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,20,0);
    track[16].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,19,0);
    track[16].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(4,18,0);
    track[16].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(4,17,0);
    track[16].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(4,16,0);
    trackGraph_copyGraphNodesToReverse(&(track[16].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[16].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B2->16
    track[17].edge[DIR_AHEAD].numGraphNodes=4;
    track[17].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,24,0);
    track[17].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,25,0);
    track[17].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,26,0);
    track[17].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[17].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[17].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B3->C2
    track[18].edge[DIR_AHEAD].numGraphNodes=3;
    track[18].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,25,0);
    track[18].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,23,0);
    track[18].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,22,0);
    trackGraph_copyGraphNodesToReverse(&(track[18].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[18].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B4->16
    track[19].edge[DIR_AHEAD].numGraphNodes=2;
    track[19].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,25,0);
    track[19].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[19].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[19].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B5->D3
    track[20].edge[DIR_AHEAD].numGraphNodes=9;
    track[20].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,24,0);
    track[20].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,23,0);
    track[20].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,22,0);
    track[20].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,21,0);
    track[20].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,20,0);
    track[20].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,19,0);
    track[20].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(14,18,0);
    track[20].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(14,17,0);
    track[20].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(14,16,0);
    trackGraph_copyGraphNodesToReverse(&(track[20].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[20].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B6->13
    track[21].edge[DIR_AHEAD].numGraphNodes=4;
    track[21].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,24,0);
    track[21].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,25,0);
    track[21].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,26,0);
    track[21].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[21].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[21].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B8->EX
    track[23].edge[DIR_AHEAD].numGraphNodes=2;
    track[23].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,52,0);
    track[23].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,53,0);
    trackGraph_copyGraphNodesToReverse(&(track[23].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[23].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B10->EX
    track[25].edge[DIR_AHEAD].numGraphNodes=2;
    track[25].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,53,0);
    track[25].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[25].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[25].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B12->EX
    track[27].edge[DIR_AHEAD].numGraphNodes=2;
    track[27].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,53,0);
    track[27].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,54,0);
    trackGraph_copyGraphNodesToReverse(&(track[27].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[27].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B13->154
    track[28].edge[DIR_AHEAD].numGraphNodes=3;
    track[28].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,18,0);
    track[28].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,19,0);
    track[28].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[28].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[28].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B14->D16
    track[29].edge[DIR_AHEAD].numGraphNodes=3;
    track[29].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,18,0);
    track[29].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,17,0);
    track[29].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(5,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[29].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[29].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B16->15
    track[31].edge[DIR_AHEAD].numGraphNodes=2;
    track[31].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,40,0);
    track[31].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[31].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[31].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C2->153
    track[33].edge[DIR_AHEAD].numGraphNodes=3;
    track[33].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,22,0);
    track[33].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,21,0);
    track[33].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[33].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[33].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C3->EX3
    track[34].edge[DIR_AHEAD].numGraphNodes=9;
    track[34].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,9,0);
    track[34].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,8,0);
    track[34].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,7,0);
    track[34].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,6,0);
    track[34].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,5,0);
    track[34].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,4,0);
    track[34].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,3,0);
    track[34].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,2,0);
    track[34].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,1,0);
    trackGraph_copyGraphNodesToReverse(&(track[34].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[34].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C4->5
    track[35].edge[DIR_AHEAD].numGraphNodes=6;
    track[35].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,9,0);
    track[35].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,10,0);
    track[35].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,11,0);
    track[35].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,12,0);
    track[35].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,13,0);
    track[35].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,14,0);
    trackGraph_copyGraphNodesToReverse(&(track[35].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[35].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C5->6
    track[36].edge[DIR_AHEAD].numGraphNodes=4;
    track[36].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,32,0);
    track[36].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,31,0);
    track[36].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,30,0);
    track[36].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,29,0);
    trackGraph_copyGraphNodesToReverse(&(track[36].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[36].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C6->15
    track[37].edge[DIR_AHEAD].numGraphNodes=5;
    track[37].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,32,0);
    track[37].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,35,0);
    track[37].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,37,0);
    track[37].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,38,0);
    track[37].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(6,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[37].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[37].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C7->18
    track[38].edge[DIR_AHEAD].numGraphNodes=6;
    track[38].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,30,0);
    track[38].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,29,0);
    track[38].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,28,0);
    track[38].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,27,0);
    track[38].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,26,0);
    track[38].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,25,0);
    trackGraph_copyGraphNodesToReverse(&(track[38].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[38].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C8->3
    track[39].edge[DIR_AHEAD].numGraphNodes=7;
    track[39].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,30,0);
    track[39].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,31,0);
    track[39].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,32,0);
    track[39].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,33,0);
    track[39].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,34,0);
    track[39].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,35,0);
    track[39].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,36,0);
    trackGraph_copyGraphNodesToReverse(&(track[39].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[39].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C9->15
    track[40].edge[DIR_AHEAD].numGraphNodes=5;
    track[40].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,32,0);
    track[40].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,33,0);
    track[40].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,34,0);
    track[40].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,37,0);
    track[40].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(6,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[40].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[40].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C10->16
    track[41].edge[DIR_AHEAD].numGraphNodes=6;
    track[41].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,32,0);
    track[41].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,31,0);
    track[41].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,30,0);
    track[41].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,29,0);
    track[41].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,28,0);
    track[41].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[41].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[41].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C11->13
    track[42].edge[DIR_AHEAD].numGraphNodes=6;
    track[42].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,32,0);
    track[42].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,31,0);
    track[42].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,30,0);
    track[42].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,29,0);
    track[42].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,28,0);
    track[42].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[42].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[42].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C12->14
    track[43].edge[DIR_AHEAD].numGraphNodes=5;
    track[43].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,32,0);
    track[43].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,33,0);
    track[43].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,34,0);
    track[43].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(13,37,0);
    track[43].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(12,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[43].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[43].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C13->E7
    track[44].edge[DIR_AHEAD].numGraphNodes=19;
    track[44].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,29,0);
    track[44].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,28,0);
    track[44].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,27,0);
    track[44].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,26,0);
    track[44].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,25,0);
    track[44].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,24,0);
    track[44].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,23,0);
    track[44].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,22,0);
    track[44].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(16,21,0);
    track[44].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(16,20,0);
    track[44].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(16,19,0);
    track[44].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(16,18,0);
    track[44].edge[DIR_AHEAD].graphNodes[12]=trackGraph_buildNode(16,17,0);
    track[44].edge[DIR_AHEAD].graphNodes[13]=trackGraph_buildNode(16,16,0);
    track[44].edge[DIR_AHEAD].graphNodes[14]=trackGraph_buildNode(16,15,0);
    track[44].edge[DIR_AHEAD].graphNodes[15]=trackGraph_buildNode(16,14,0);
    track[44].edge[DIR_AHEAD].graphNodes[16]=trackGraph_buildNode(16,13,0);
    track[44].edge[DIR_AHEAD].graphNodes[17]=trackGraph_buildNode(16,12,0);
    track[44].edge[DIR_AHEAD].graphNodes[18]=trackGraph_buildNode(16,11,0);
    trackGraph_copyGraphNodesToReverse(&(track[44].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[44].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C14->11
    track[45].edge[DIR_AHEAD].numGraphNodes=4;
    track[45].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,29,0);
    track[45].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,30,0);
    track[45].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,31,0);
    track[45].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,32,0);
    trackGraph_copyGraphNodesToReverse(&(track[45].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[45].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C15->D12
    track[46].edge[DIR_AHEAD].numGraphNodes=12;
    track[46].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,25,0);
    track[46].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,24,0);
    track[46].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,23,0);
    track[46].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,22,0);
    track[46].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,21,0);
    track[46].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(2,20,0);
    track[46].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(2,19,0);
    track[46].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(2,18,0);
    track[46].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(2,17,0);
    track[46].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(2,16,0);
    track[46].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(2,15,0);
    track[46].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(2,14,0);
    trackGraph_copyGraphNodesToReverse(&(track[46].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[46].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // C16->6
    track[47].edge[DIR_AHEAD].numGraphNodes=5;
    track[47].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,25,0);
    track[47].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,26,0);
    track[47].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,27,0);
    track[47].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,28,0);
    track[47].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,29,0);
    trackGraph_copyGraphNodesToReverse(&(track[47].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[47].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D1->155
    track[48].edge[DIR_AHEAD].numGraphNodes=3;
    track[48].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,18,0);
    track[48].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,19,0);
    track[48].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[48].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[48].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D2->E4
    track[49].edge[DIR_AHEAD].numGraphNodes=3;
    track[49].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,18,0);
    track[49].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,17,0);
    track[49].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(13,15,0);
    trackGraph_copyGraphNodesToReverse(&(track[49].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[49].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D3->10
    track[50].edge[DIR_AHEAD].numGraphNodes=4;
    track[50].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,16,0);
    track[50].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,15,0);
    track[50].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,14,0);
    track[50].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[50].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[50].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D5->E6
    track[52].edge[DIR_AHEAD].numGraphNodes=6;
    track[52].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,4,0);
    track[52].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,6,0);
    track[52].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,7,0);
    track[52].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,8,0);
    track[52].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,9,0);
    track[52].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(14,10,0);
    trackGraph_copyGraphNodesToReverse(&(track[52].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[52].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D6->9
    track[53].edge[DIR_AHEAD].numGraphNodes=3;
    track[53].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,4,0);
    track[53].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,2,0);
    track[53].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(11,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[53].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[53].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D7->9
    track[54].edge[DIR_AHEAD].numGraphNodes=4;
    track[54].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,3,0);
    track[54].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,2,0);
    track[54].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,1,0);
    track[54].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[54].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[54].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D8->E8
    track[55].edge[DIR_AHEAD].numGraphNodes=7;
    track[55].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,3,0);
    track[55].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,4,0);
    track[55].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,7,0);
    track[55].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,8,0);
    track[55].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,9,0);
    track[55].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,10,0);
    track[55].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,11,0);
    trackGraph_copyGraphNodesToReverse(&(track[55].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[55].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D9->E12
    track[56].edge[DIR_AHEAD].numGraphNodes=3;
    track[56].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,3,0);
    track[56].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,4,0);
    track[56].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,7,0);
    trackGraph_copyGraphNodesToReverse(&(track[56].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[56].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D10->8
    track[57].edge[DIR_AHEAD].numGraphNodes=4;
    track[57].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,3,0);
    track[57].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(5,2,0);
    track[57].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(6,1,0);
    track[57].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[57].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[57].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D12->7
    track[59].edge[DIR_AHEAD].numGraphNodes=5;
    track[59].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,14,0);
    track[59].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,13,0);
    track[59].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,12,0);
    track[59].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,11,0);
    track[59].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(2,10,0);
    trackGraph_copyGraphNodesToReverse(&(track[59].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[59].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D14->17
    track[61].edge[DIR_AHEAD].numGraphNodes=4;
    track[61].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,16,0);
    track[61].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,15,0);
    track[61].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,14,0);
    track[61].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[61].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[61].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // D16->17
    track[63].edge[DIR_AHEAD].numGraphNodes=2;
    track[63].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,15,0);
    track[63].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[63].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[63].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E1->156
    track[64].edge[DIR_AHEAD].numGraphNodes=3;
    track[64].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,22,0);
    track[64].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,21,0);
    track[64].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[64].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[64].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E2->15
    track[65].edge[DIR_AHEAD].numGraphNodes=3;
    track[65].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,22,0);
    track[65].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,23,0);
    track[65].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(13,25,0);
    trackGraph_copyGraphNodesToReverse(&(track[65].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[65].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E4->10
    track[67].edge[DIR_AHEAD].numGraphNodes=2;
    track[67].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,15,0);
    track[67].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[67].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[67].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E6->10
    track[69].edge[DIR_AHEAD].numGraphNodes=4;
    track[69].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10,0);
    track[69].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,11,0);
    track[69].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,12,0);
    track[69].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[69].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[69].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E9->8
    track[72].edge[DIR_AHEAD].numGraphNodes=3;
    track[72].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,4,0);
    track[72].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,2,0);
    track[72].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[72].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[72].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E10->E13
    track[73].edge[DIR_AHEAD].numGraphNodes=6;
    track[73].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,4,0);
    track[73].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,6,0);
    track[73].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,7,0);
    track[73].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,8,0);
    track[73].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,9,0);
    track[73].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(4,10,0);
    trackGraph_copyGraphNodesToReverse(&(track[73].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[73].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E12->7
    track[75].edge[DIR_AHEAD].numGraphNodes=4;
    track[75].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,7,0);
    track[75].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,8,0);
    track[75].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,9,0);
    track[75].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,10,0);
    trackGraph_copyGraphNodesToReverse(&(track[75].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[75].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E13->17
    track[76].edge[DIR_AHEAD].numGraphNodes=4;
    track[76].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,10,0);
    track[76].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,11,0);
    track[76].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,12,0);
    track[76].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(4,13,0);
    trackGraph_copyGraphNodesToReverse(&(track[76].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[76].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E15->13
    track[78].edge[DIR_AHEAD].numGraphNodes=2;
    track[78].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(13,25,0);
    track[78].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,27,0);
    trackGraph_copyGraphNodesToReverse(&(track[78].edge[DIR_AHEAD]));
    // trackGraph_colorEdge(&(track[78].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // MR1->2
    track[81].edge[DIR_AHEAD].numGraphNodes=3;
    track[81].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,45,0);
    track[81].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,43,0);
    track[81].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,41,0);
    trackGraph_copyGraphNodesToReverse(&(track[81].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[81].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR2->3
    track[83].edge[DIR_AHEAD].numGraphNodes=3;
    track[83].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,41,0);
    track[83].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,39,0);
    track[83].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,36,0);
    trackGraph_copyGraphNodesToReverse(&(track[83].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[83].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR4->12
    track[87].edge[DIR_AHEAD].numGraphNodes=3;
    track[87].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,44,0);///
    track[87].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,41,0);
    track[87].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,37,0);
    trackGraph_copyGraphNodesToReverse(&(track[87].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[87].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR5->18
    track[89].edge[DIR_AHEAD].numGraphNodes=12;
    track[89].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25,0);
    track[89].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,24,0);
    track[89].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,23,0);
    track[89].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,22,0);
    track[89].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(0,21,0);
    track[89].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(0,20,0);
    track[89].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(0,19,0);
    track[89].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(0,18,0);
    track[89].edge[DIR_AHEAD].graphNodes[8]=trackGraph_buildNode(0,17,0);
    track[89].edge[DIR_AHEAD].graphNodes[9]=trackGraph_buildNode(0,16,0);
    track[89].edge[DIR_AHEAD].graphNodes[10]=trackGraph_buildNode(0,15,0);
    track[89].edge[DIR_AHEAD].graphNodes[11]=trackGraph_buildNode(0,14,0);
    trackGraph_copyGraphNodesToReverse(&(track[89].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[89].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // BR6CURVED->18
    track[90].edge[DIR_CURVED].numGraphNodes=3;
    track[90].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(2,29,0);
    track[90].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,27,0);
    track[90].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(0,25,0);
    trackGraph_copyGraphNodesToReverse(&(track[90].edge[DIR_CURVED]));
    // trackGraph_colorEdge(&(track[90].edge[DIR_CURVED]),whiteColor);
    IOidle(COM2);

    // BR7CURVED->5
    track[92].edge[DIR_CURVED].numGraphNodes=3;
    track[92].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(2,10,0);
    track[92].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,12,0);
    track[92].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(0,14,0);
    trackGraph_copyGraphNodesToReverse(&(track[92].edge[DIR_CURVED]));
    // trackGraph_colorEdge(&(track[92].edge[DIR_CURVED]),whiteColor);
    IOidle(COM2);

    // MR9->MR8
    track[97].edge[DIR_AHEAD].numGraphNodes=5;
    track[97].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,0,0);
    track[97].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,0,0);
    track[97].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,0,0);
    track[97].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,0,0);
    track[97].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(7,0,0);
    trackGraph_copyGraphNodesToReverse(&(track[97].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[97].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // BR11CURVEd->14
    track[100].edge[DIR_CURVED].numGraphNodes=5;
    track[100].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(16,32,0);
    track[100].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,35,0);
    track[100].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(14,37,0);
    track[100].edge[DIR_CURVED].graphNodes[3]=trackGraph_buildNode(13,38,0);
    track[100].edge[DIR_CURVED].graphNodes[4]=trackGraph_buildNode(12,39,0);
    trackGraph_copyGraphNodesToReverse(&(track[100].edge[DIR_CURVED]));
    // trackGraph_colorEdge(&(track[100].edge[DIR_CURVED]),whiteColor);
    IOidle(COM2);

    // MR12->11
    track[103].edge[DIR_AHEAD].numGraphNodes=8;
    track[103].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,39,0);
    track[103].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,38,0);
    track[103].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,37,0);
    track[103].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(16,36,0);
    track[103].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(16,35,0);
    track[103].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(16,34,0);
    track[103].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(16,33,0);
    track[103].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(16,32,0);
    trackGraph_copyGraphNodesToReverse(&(track[103].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[103].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // MR153->154
    track[117].edge[DIR_AHEAD].numGraphNodes=2;
    track[117].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20,0);
    track[117].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[117].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR154->156
    track[119].edge[DIR_AHEAD].numGraphNodes=2;
    track[119].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20,0);
    track[119].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[119].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR155->156
    track[121].edge[DIR_AHEAD].numGraphNodes=2;
    track[121].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20,0);
    track[121].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[121].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // MR156->154
    track[123].edge[DIR_AHEAD].numGraphNodes=2;
    track[123].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,20,0);
    track[123].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[123].edge[DIR_STRAIGHT]));
    IOidle(COM2);

    // EN1->155
    track[124].edge[DIR_AHEAD].numGraphNodes=4;
    track[124].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(6,20,0);
    track[124].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(7,20,0);
    track[124].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(8,20,0);
    track[124].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[124].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[126].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);

    // EN2->153
    track[126].edge[DIR_AHEAD].numGraphNodes=4;
    track[126].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,20,0);
    track[126].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(11,20,0);
    track[126].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,20,0);
    track[126].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,20,0);
    trackGraph_copyGraphNodesToReverse(&(track[126].edge[DIR_STRAIGHT]));
    // trackGraph_colorEdge(&(track[124].edge[DIR_STRAIGHT]),whiteColor);
    IOidle(COM2);


    /* Initialize switch in graph. */
    int i = 0;
    for ( ; i < SWITCH_TOTAL; i++) {
        trackGraph_colorSwitch(&(track[i * 2 + 80].edge[data->swtable[i]]), SW_HIGHLIGHT_COLOR);
    }
}

void trackGraph_extractNodeInfo(unsigned int graphNodeInfo, int *row, int *col, int *isSwPath) {
    *row = graphNodeInfo % 64;
    graphNodeInfo = graphNodeInfo / 64;
    *col = graphNodeInfo % 64;
    graphNodeInfo = graphNodeInfo / 64;
    *isSwPath = graphNodeInfo;
}

unsigned int trackGraph_buildNode(int row, int col, int isSwPath) {

    assert(isSwPath == 0 || isSwPath == 1, "trackGraph_buildNode: isSwPath not 0 or 1");
    return row + col * 64 + isSwPath * 64 * 64;
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
