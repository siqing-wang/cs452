#include <track_graph.h>
#include <ui.h>
#include <syscall.h>
#include <utils.h>
#include <trainset.h>

/* Have each graph node's info inside a signle int to save space. */
void trackGraph_extractNodeInfo(unsigned int graphNodeInfo, int *row, int *col);
unsigned int trackGraph_buildNode(int row, int col);

/* Initialization Helper. */
void trackGraph_copyGraphNodesToReverse(track_edge *edge);


void trackGraph_colorEdge(track_edge *edge, int color) {
    int row, col;

    int i;
    PrintfAt(COM2, 0, 0, "\033[%dm", color);
    for (i = 0; i < edge->numGraphNodes; i++) {
        trackGraph_extractNodeInfo(edge->graphNodes[i], &row, &col);
        PrintfAt(COM2, TRACK_R+row, TRACK_C+col, "*");
        IOidle(COM2);
    }
    PutStr(COM2, TCS_RESET);
}

void trackGraph_colorTillNextSensor(struct TrainSetData *data, track_node *node, int color) {

    int *swtable = data->swtable;
    int corresSwNo, direction;
    track_node *destNode = node;

    for (;;) {
        /* Color until reaching another sensor. */
        direction = 0;
        if (destNode->type == NODE_BRANCH) {
            /* Get branch's current direction from switch table. */
            corresSwNo = destNode->num;
            direction = *(swtable + getSwitchIndex(corresSwNo));
        }
        PrintfAt(COM2, 1,1, "Color %s", destNode->name);
        trackGraph_colorEdge(&(destNode->edge[direction]), color);
        destNode = destNode->edge[direction].dest;

        if (destNode != node && destNode->type == NODE_SENSOR) {
            break;
        } else if (destNode->type == NODE_EXIT) {
            break;
        }
    }
}

void trackGraph_init(track_node *track) {
    int whiteColor = 37;

    // /* Draw Track. */
    // PrintfAt(COM2, TRACK_R, TRACK_C,     "***************2******1**c*****************e****"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+1, TRACK_C,   "             *     *                              *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+2, TRACK_C,   "************     *  **c****3**b*******d**0***c***  d"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+3, TRACK_C,   "          *     **           e         e          d *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+4, TRACK_C,   " ********      4               *  *  *              **"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+5, TRACK_C,   "              *                 e * d                 9"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+6, TRACK_C,   "              *                  ***                  *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+7, TRACK_C,   "              *                   *                   *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+8, TRACK_C,   "              *                  ***                  *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+9, TRACK_C,   "              *                 c * b                 8"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+10, TRACK_C,  " ********      b               *  *  *              **"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+11, TRACK_C,  "         *      **           b         d          e *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+12, TRACK_C,  " **********      *  *****************************  d"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+13, TRACK_C,  "           *       *                              *"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+14, TRACK_C,  "**************        ***x******************x***"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+15, TRACK_C,  "               *           x              x"); IOidle(COM2);
    // PrintfAt(COM2, TRACK_R+16, TRACK_C,  "******************3*****c****x**********x****c********"); IOidle(COM2);

    // A1 -> 12
    track[0].edge[DIR_AHEAD].numGraphNodes = 5;
    track[0].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(0, 11);
    track[0].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(0, 12);
    track[0].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(0, 13);
    track[0].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(0, 14);
    track[0].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(0, 15);
    trackGraph_copyGraphNodesToReverse(&(track[0].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[0].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[1].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A3 -> 14
    track[2].edge[DIR_AHEAD].numGraphNodes=2;
    track[2].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,14);
    track[2].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,15);
    trackGraph_copyGraphNodesToReverse(&(track[2].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[2].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A4 -> B16
    track[3].edge[DIR_AHEAD].numGraphNodes=5;
    track[3].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,14);
    track[3].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,14);
    track[3].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,14);
    track[3].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,14);
    track[3].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,14);
    trackGraph_copyGraphNodesToReverse(&(track[3].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[3].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[4].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[5].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[6].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A8 -> 2
    track[7].edge[DIR_AHEAD].numGraphNodes=3;
    track[7].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,10);
    track[7].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,11);
    track[7].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,12);
    trackGraph_copyGraphNodesToReverse(&(track[7].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[7].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[8].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A10 -> 1
    track[9].edge[DIR_AHEAD].numGraphNodes=3;
    track[9].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,7);
    track[9].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,8);
    track[9].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,9);
    trackGraph_copyGraphNodesToReverse(&(track[9].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[9].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);



    // A11 -> 1
    track[10].edge[DIR_AHEAD].numGraphNodes=8;
    track[10].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,2);
    track[10].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,3);
    track[10].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,4);
    track[10].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(10,5);
    track[10].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,6);
    track[10].edge[DIR_AHEAD].graphNodes[5]=trackGraph_buildNode(10,7);
    track[10].edge[DIR_AHEAD].graphNodes[6]=trackGraph_buildNode(10,8);
    track[10].edge[DIR_AHEAD].graphNodes[7]=trackGraph_buildNode(11,9);
    trackGraph_copyGraphNodesToReverse(&(track[10].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[10].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A12 -> EX
    track[11].edge[DIR_AHEAD].numGraphNodes=2;
    track[11].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(10,2);
    track[11].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,1);
    trackGraph_copyGraphNodesToReverse(&(track[11].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[11].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A13 -> 4
    track[12].edge[DIR_AHEAD].numGraphNodes = 4;
    track[12].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(2, 8);
    track[12].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(2, 9);
    track[12].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(2, 10);
    track[12].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(2, 11);
    trackGraph_copyGraphNodesToReverse(&(track[12].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[12].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[13].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // A15 -> EX
    track[14].edge[DIR_AHEAD].numGraphNodes = 5;
    track[14].edge[DIR_AHEAD].graphNodes[0] = trackGraph_buildNode(4, 5);
    track[14].edge[DIR_AHEAD].graphNodes[1] = trackGraph_buildNode(4, 4);
    track[14].edge[DIR_AHEAD].graphNodes[2] = trackGraph_buildNode(4, 3);
    track[14].edge[DIR_AHEAD].graphNodes[3] = trackGraph_buildNode(4, 2);
    track[14].edge[DIR_AHEAD].graphNodes[4] = trackGraph_buildNode(4, 1);
    trackGraph_copyGraphNodesToReverse(&(track[14].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[14].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[15].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[16].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B2 -> 16
    track[17].edge[DIR_AHEAD].numGraphNodes=4;
    track[17].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,30);
    track[17].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,29);
    track[17].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,28);
    track[17].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,27);
    trackGraph_copyGraphNodesToReverse(&(track[17].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[17].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // B3 -> C2
    track[18].edge[DIR_AHEAD].numGraphNodes=3;
    track[18].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,29);
    track[18].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,31);
    track[18].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,32);
    trackGraph_copyGraphNodesToReverse(&(track[18].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[18].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[20].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B6 -> 13
    track[21].edge[DIR_AHEAD].numGraphNodes=4;
    track[21].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,30);
    track[21].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,29);
    track[21].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,28);
    track[21].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,27);
    trackGraph_copyGraphNodesToReverse(&(track[21].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[21].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B8 -> EX
    track[23].edge[DIR_AHEAD].numGraphNodes=2;
    track[23].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,2);
    track[23].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,1);
    trackGraph_copyGraphNodesToReverse(&(track[23].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[23].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B10 -> EX
    track[25].edge[DIR_AHEAD].numGraphNodes=2;
    track[25].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(16,1);
    track[25].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(16,0);
    trackGraph_copyGraphNodesToReverse(&(track[25].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[25].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B12 -> EX
    track[27].edge[DIR_AHEAD].numGraphNodes=2;
    track[27].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,1);
    track[27].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,0);
    trackGraph_copyGraphNodesToReverse(&(track[27].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[27].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B13 -> 154
    track[28].edge[DIR_AHEAD].numGraphNodes=3;
    track[28].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,36);
    track[28].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,35);
    track[28].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[28].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[28].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B14 -> D16
    track[29].edge[DIR_AHEAD].numGraphNodes=3;
    track[29].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,36);
    track[29].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,37);
    track[29].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(11,39);
    trackGraph_copyGraphNodesToReverse(&(track[29].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[29].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // B16 -> 15
    track[31].edge[DIR_AHEAD].numGraphNodes=2;
    track[31].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,14);
    track[31].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,15);
    trackGraph_copyGraphNodesToReverse(&(track[31].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[31].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C2 -> 153
    track[33].edge[DIR_AHEAD].numGraphNodes=3;
    track[33].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(9,32);
    track[33].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,33);
    track[33].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[33].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[33].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[34].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[35].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C5 -> 6
    track[36].edge[DIR_AHEAD].numGraphNodes=4;
    track[36].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,22);
    track[36].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,23);
    track[36].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,24);
    track[36].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,25);
    trackGraph_copyGraphNodesToReverse(&(track[36].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[36].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C6 -> 15
    track[37].edge[DIR_AHEAD].numGraphNodes=5;
    track[37].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,22);
    track[37].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,19);
    track[37].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,17);
    track[37].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,16);
    track[37].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,15);
    trackGraph_copyGraphNodesToReverse(&(track[37].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[37].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[38].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[39].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C9 -> 15
    track[40].edge[DIR_AHEAD].numGraphNodes=5;
    track[40].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,22);
    track[40].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,21);
    track[40].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,20);
    track[40].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(11,17);
    track[40].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(10,15);
    trackGraph_copyGraphNodesToReverse(&(track[40].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[40].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[41].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[42].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C12 -> 14
    track[43].edge[DIR_AHEAD].numGraphNodes=5;
    track[43].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,22);
    track[43].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,21);
    track[43].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,20);
    track[43].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(3,17);
    track[43].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(4,15);
    trackGraph_copyGraphNodesToReverse(&(track[43].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[43].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[44].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C14 -> 11
    track[45].edge[DIR_AHEAD].numGraphNodes=4;
    track[45].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(0,25);
    track[45].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(0,24);
    track[45].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,23);
    track[45].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(0,22);
    trackGraph_copyGraphNodesToReverse(&(track[45].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[45].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[46].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // C16 -> 6
    track[47].edge[DIR_AHEAD].numGraphNodes=5;
    track[47].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,29);
    track[47].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,28);
    track[47].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,27);
    track[47].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,26);
    track[47].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,25);
    trackGraph_copyGraphNodesToReverse(&(track[47].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[47].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D1 -> 155
    track[48].edge[DIR_AHEAD].numGraphNodes=3;
    track[48].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,36);
    track[48].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,35);
    track[48].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[48].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[48].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D2 -> E4
    track[49].edge[DIR_AHEAD].numGraphNodes=3;
    track[49].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,36);
    track[49].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,37);
    track[49].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(3,39);
    trackGraph_copyGraphNodesToReverse(&(track[49].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[49].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D3 -> 10
    track[50].edge[DIR_AHEAD].numGraphNodes=4;
    track[50].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,38);
    track[50].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,39);
    track[50].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,40);
    track[50].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[50].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[50].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[52].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D6 -> 9
    track[53].edge[DIR_AHEAD].numGraphNodes=3;
    track[53].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,50);
    track[53].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,52);
    track[53].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(5,54);
    trackGraph_copyGraphNodesToReverse(&(track[53].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[53].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D7 -> 9
    track[54].edge[DIR_AHEAD].numGraphNodes=4;
    track[54].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,51);
    track[54].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(3,52);
    track[54].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(4,53);
    track[54].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(5,54);
    trackGraph_copyGraphNodesToReverse(&(track[54].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[54].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[55].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D9 -> E12
    track[56].edge[DIR_AHEAD].numGraphNodes=3;
    track[56].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,51);
    track[56].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,50);
    track[56].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,47);
    trackGraph_copyGraphNodesToReverse(&(track[56].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[56].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D10 -> 8
    track[57].edge[DIR_AHEAD].numGraphNodes=4;
    track[57].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,51);
    track[57].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(11,52);
    track[57].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(10,53);
    track[57].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(9,54);
    trackGraph_copyGraphNodesToReverse(&(track[57].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[57].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D12 -> 7
    track[59].edge[DIR_AHEAD].numGraphNodes=5;
    track[59].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,40);
    track[59].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,41);
    track[59].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,42);
    track[59].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,43);
    track[59].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(14,44);
    trackGraph_copyGraphNodesToReverse(&(track[59].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[59].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // D14 -> 17
    track[61].edge[DIR_AHEAD].numGraphNodes=4;
    track[61].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,38);
    track[61].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,39);
    track[61].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,40);
    track[61].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,41);
    trackGraph_copyGraphNodesToReverse(&(track[61].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[61].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E1 -> 156
    track[64].edge[DIR_AHEAD].numGraphNodes=3;
    track[64].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,32);
    track[64].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,33);
    track[64].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[64].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[64].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E2 -> 15
    track[65].edge[DIR_AHEAD].numGraphNodes=3;
    track[65].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,32);
    track[65].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(4,31);
    track[65].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(3,29);
    trackGraph_copyGraphNodesToReverse(&(track[65].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[65].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E4 -> 10
    track[67].edge[DIR_AHEAD].numGraphNodes=2;
    track[67].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,39);
    track[67].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[67].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[67].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E6 -> 10
    track[69].edge[DIR_AHEAD].numGraphNodes=4;
    track[69].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,44);
    track[69].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,43);
    track[69].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(2,42);
    track[69].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(2,41);
    trackGraph_copyGraphNodesToReverse(&(track[69].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[69].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E9 -> 8
    track[72].edge[DIR_AHEAD].numGraphNodes=3;
    track[72].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(11,50);
    track[72].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(10,52);
    track[72].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,54);
    trackGraph_copyGraphNodesToReverse(&(track[72].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[72].edge[DIR_AHEAD]), whiteColor);
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
    trackGraph_colorEdge(&(track[73].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E12 -> 7
    track[75].edge[DIR_AHEAD].numGraphNodes=4;
    track[75].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,47);
    track[75].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(14,46);
    track[75].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,45);
    track[75].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(14,44);
    trackGraph_copyGraphNodesToReverse(&(track[75].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[75].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // E13 -> 17
    track[76].edge[DIR_AHEAD].numGraphNodes=4;
    track[76].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,44);
    track[76].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(12,43);
    track[76].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(12,42);
    track[76].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(12,41);
    trackGraph_copyGraphNodesToReverse(&(track[76].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[76].edge[DIR_AHEAD]),whiteColor);
    IOidle(COM2);

    // E15 -> 13
    track[78].edge[DIR_AHEAD].numGraphNodes=2;
    track[78].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(3,29);
    track[78].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(2,27);
    trackGraph_copyGraphNodesToReverse(&(track[78].edge[DIR_AHEAD]));
    trackGraph_colorEdge(&(track[78].edge[DIR_AHEAD]), whiteColor);
    IOidle(COM2);

    // MR1 -> 2
    track[81].edge[DIR_AHEAD].numGraphNodes=3;
    track[81].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(12,10);
    track[81].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(13,11);
    track[81].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(14,13);
    trackGraph_copyGraphNodesToReverse(&(track[81].edge[DIR_STRAIGHT]));
    trackGraph_colorEdge(&(track[81].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR2 -> 3
    track[83].edge[DIR_AHEAD].numGraphNodes=3;
    track[83].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(14,13);
    track[83].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(15,15);
    track[83].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(16,18);
    trackGraph_copyGraphNodesToReverse(&(track[83].edge[DIR_STRAIGHT]));
    trackGraph_colorEdge(&(track[83].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // MR4 -> 12
    track[87].edge[DIR_AHEAD].numGraphNodes=3;
    track[87].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(2,11);
    track[87].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(1,13);
    track[87].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(0,15);
    trackGraph_copyGraphNodesToReverse(&(track[87].edge[DIR_STRAIGHT]));
    trackGraph_colorEdge(&(track[87].edge[DIR_STRAIGHT]), whiteColor);
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
    trackGraph_colorEdge(&(track[89].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // BR6 CURVED-> 18
    track[90].edge[DIR_CURVED].numGraphNodes=3;
    track[90].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(14,25);
    track[90].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,27);
    track[90].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(16,29);
    trackGraph_copyGraphNodesToReverse(&(track[90].edge[DIR_CURVED]));
    trackGraph_colorEdge(&(track[90].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // BR7 CURVED-> 5
    track[92].edge[DIR_CURVED].numGraphNodes=3;
    track[92].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(14,44);
    track[92].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(15,42);
    track[92].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(16,40);
    trackGraph_copyGraphNodesToReverse(&(track[92].edge[DIR_CURVED]));
    trackGraph_colorEdge(&(track[92].edge[DIR_CURVED]), whiteColor);
    IOidle(COM2);

    // MR9 -> MR8
    track[97].edge[DIR_AHEAD].numGraphNodes=5;
    track[97].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(5,54);
    track[97].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(6,54);
    track[97].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(7,54);
    track[97].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(8,54);
    track[97].edge[DIR_AHEAD].graphNodes[4]=trackGraph_buildNode(9,54);
    trackGraph_copyGraphNodesToReverse(&(track[97].edge[DIR_STRAIGHT]));
    trackGraph_colorEdge(&(track[97].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // BR11 CURVEd-> 14
    track[100].edge[DIR_CURVED].numGraphNodes=5;
    track[100].edge[DIR_CURVED].graphNodes[0]=trackGraph_buildNode(0,22);
    track[100].edge[DIR_CURVED].graphNodes[1]=trackGraph_buildNode(1,19);
    track[100].edge[DIR_CURVED].graphNodes[2]=trackGraph_buildNode(2,17);
    track[100].edge[DIR_CURVED].graphNodes[3]=trackGraph_buildNode(3,16);
    track[100].edge[DIR_CURVED].graphNodes[4]=trackGraph_buildNode(4,15);
    trackGraph_copyGraphNodesToReverse(&(track[100].edge[DIR_CURVED]));
    trackGraph_colorEdge(&(track[100].edge[DIR_CURVED]), whiteColor);
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
    trackGraph_colorEdge(&(track[103].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // EN1 -> 153
    track[124].edge[DIR_AHEAD].numGraphNodes=4;
    track[124].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(4,34);
    track[124].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(5,34);
    track[124].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(6,34);
    track[124].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(7,34);
    trackGraph_copyGraphNodesToReverse(&(track[124].edge[DIR_STRAIGHT]));
    trackGraph_colorEdge(&(track[124].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);

    // EN1 -> 153
    track[126].edge[DIR_AHEAD].numGraphNodes=4;
    track[126].edge[DIR_AHEAD].graphNodes[0]=trackGraph_buildNode(7,34);
    track[126].edge[DIR_AHEAD].graphNodes[1]=trackGraph_buildNode(8,34);
    track[126].edge[DIR_AHEAD].graphNodes[2]=trackGraph_buildNode(9,34);
    track[126].edge[DIR_AHEAD].graphNodes[3]=trackGraph_buildNode(10,34);
    trackGraph_copyGraphNodesToReverse(&(track[126].edge[DIR_STRAIGHT]));
    trackGraph_colorEdge(&(track[126].edge[DIR_STRAIGHT]), whiteColor);
    IOidle(COM2);
}

void trackGraph_extractNodeInfo(unsigned int graphNodeInfo, int *row, int *col) {
    *row = graphNodeInfo % 64;
    graphNodeInfo = graphNodeInfo / 64;
    *col = graphNodeInfo % 64;
}

unsigned int trackGraph_buildNode(int row, int col) {
    return row + col * 64;
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
