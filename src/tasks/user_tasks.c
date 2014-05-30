 /*
 * user_task.c
 */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <message.h>
#include <bwio.h>
#include <utils.h>

#define NUM_PLAYER 4
#define OPPONENT_SWITCHED "Opponent Switched."
#define NO_OPPONENT "No Opponent Left."

void rpsServer() {
    RegisterAs("RPS Server");

    RPSMessage message;
    int playerQueue[NUM_PLAYER];
    int playerQueueIndex = 0;
    int playerCount = 0;
    int nextPlayer = 0;

    int player1Tid = -1;
    int player2Tid = -1;
    int player1Choice = -1;
    int player2Choice = -1;

    int tid;
    int inGame = 0;
    int sendPlayerLeftMsg = 0;
    int syscallResult = 0;

    for(;;) {
        int byteSent = Receive(&tid, &message, sizeof(RPSMessage));
        if (byteSent > sizeof(RPSMessage)) {
            warning("RPSMessage Request overflowed.");
        }

        switch (message.type) {
            case RPSMSG_SIGNUP :
                playerQueue[playerQueueIndex] = tid;
                playerQueueIndex ++;
                playerCount ++;

                if (playerCount == 1) {
                    player1Tid = tid;
                }
                else if (playerCount == 2) {
                    player2Tid = tid;
                    Reply(player1Tid, "Game start.", 32);
                    Reply(player2Tid, "Game start.", 32);
                    inGame = 1;
                    nextPlayer = 2;
                }
                break;
            case RPSMSG_PLAY :
                if (tid == player1Tid) {
                    player1Choice = message.choice;
                }
                else if (tid == player2Tid) {
                    player2Choice = message.choice;
                }

                if (sendPlayerLeftMsg && (tid == player1Tid)) {
                    if (playerCount < 2) {
                        Reply(tid, NO_OPPONENT, 32);
                    }
                    else {
                        Reply(tid, OPPONENT_SWITCHED, 32);
                    }
                    sendPlayerLeftMsg = 0;
                    player1Choice = -1;
                    player2Choice = -1;
                }

                if ((player1Choice != -1) && (player2Choice != -1)) {
                    if (player1Choice == player2Choice) {
                        Reply(player1Tid, "Tier.", 32);
                        Reply(player2Tid, "Tier.", 32);
                        bwprintf(COM2, "Tier\n\r");
                    }
                    else if ((player1Choice + 1) % 3 == player2Choice) {
                        Reply(player1Tid, "Lose.", 32);
                        Reply(player2Tid, "Win.", 32);
                        bwprintf(COM2, "Task%d Win.\n\r", player2Tid);
                    }
                    else {
                        Reply(player1Tid, "Win.", 32);
                        Reply(player2Tid, "Lose.", 32);
                        bwprintf(COM2, "Task%d Win.\n\r", player1Tid);
                    }
                    bwgetc(COM2);
                    player1Choice = -1;
                    player2Choice = -1;
                }
                break;
            case RPSMSG_QUIT :
                playerCount --;
                Reply(tid, "Game Finish.", 32);
                if (tid == player1Tid) {
                    player1Tid = player2Tid;
                }

                if (playerCount >= 2) {
                    player2Tid = playerQueue[nextPlayer];
                    Reply(player2Tid, "Game start.", 32);
                    nextPlayer ++;
                    syscallResult = Reply(player1Tid, OPPONENT_SWITCHED, 32);
                }
                else {
                    syscallResult = Reply(player1Tid, NO_OPPONENT, 32);
                }
                if (syscallResult < 0) {
                    sendPlayerLeftMsg = 1;
                }

                player1Choice = -1;
                player2Choice = -1;
                break;
            default :
                warning("Unknown RPSMessage Type.");
        }
    }

    warning("NameServer Finished.");
    Exit();
}

void rpsClient() {
    int myTid = MyTid();
    int serverTid = WhoIs("RPS Server");

    RPSMessage message;
    char reply[32];

    unsigned long seed = myTid;

    message.type = RPSMSG_SIGNUP;
    Send(serverTid, &message, sizeof(RPSMessage), reply, 32);
    bwprintf(COM2, "Task%d Join the Game\n\r", myTid);

    Pass();

    for(;;) {
        seed = rand(seed);
        switch(seed % 7) {
            case 0 :
            case 1:
                message.type = RPSMSG_PLAY;
                message.choice = RPSMSG_ROCK;
                bwprintf(COM2, "Task%d : Rock.\n\r", myTid);
                Send(serverTid, &message, sizeof(RPSMessage), reply, 32);
                // bwprintf(COM2, "Task%d : %s\n\r", myTid, reply);
                break;
            case 2 :
            case 3 :
                message.type = RPSMSG_PLAY;
                message.choice = RPSMSG_PAPER;
                bwprintf(COM2, "Task%d : Paper.\n\r", myTid);
                Send(serverTid, &message, sizeof(RPSMessage), reply, 32);
                // bwprintf(COM2, "Task%d : %s\n\r", myTid, reply);
                break;
            case 4 :
            case 5 :
                message.type = RPSMSG_PLAY;
                message.choice = RPSMSG_SCISSORS;
                bwprintf(COM2, "Task%d : Scissors.\n\r", myTid);
                Send(serverTid, &message, sizeof(RPSMessage), reply, 32);
                // bwprintf(COM2, "Task%d : %s\n\r", myTid, reply);
                break;
            case 6 :
                message.type = RPSMSG_QUIT;
                bwprintf(COM2, "Task%d : Quit.\n\r", myTid);
                Send(serverTid, &message, sizeof(RPSMessage), reply, 32);
                bwprintf(COM2, "Task%d Leave the Game\n\r", myTid);
                Exit();
                break;
        }
        if (stringEquals(reply, OPPONENT_SWITCHED)) {
            bwprintf(COM2, "Task%d : %s\n\r", myTid, OPPONENT_SWITCHED);
        }
        Pass();
        if (stringEquals(reply, NO_OPPONENT)) {
            bwprintf(COM2, "Task%d : %s\n\r", myTid, NO_OPPONENT);
            message.type = RPSMSG_QUIT;
            bwprintf(COM2, "Task%d : Quit.\n\r", myTid);
            Send(serverTid, &message, sizeof(RPSMessage), reply, 32);
            bwprintf(COM2, "Task%d Leave the Game\n\r", myTid);
            Exit();
        }
    }
}

void firstUserTask() {
    int tid;

    // Create NameServer
    tid = Create(PRIORITY_HIGH, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // Create RPS Server
    tid = Create(PRIORITY_MED + 2, &rpsServer);

    // Create RPS Client1
    int i = 0;
    for(; i < NUM_PLAYER; i++) {
        tid = Create(PRIORITY_MED - 1, &rpsClient);
    }

    Exit();
}
