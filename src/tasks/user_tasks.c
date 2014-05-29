 /*
 * user_task.c
 */

#include <user_tasks.h>
#include <syscall.h>
#include <nameserver.h>
#include <message.h>
#include <bwio.h>
#include <utils.h>

void rpsServer() {
    bwprintf(COM2, "---RPS Server Initialized---\n\r");
    RegisterAs("RPS Server");
    bwprintf(COM2, "---RPS Server Registered---\n\r");

    RPSMessage message;
    int client1Tid = -1;
    int client2Tid = -1;
    int tid;
    for(;;) {
        int byteSent = Receive(&tid, &message, sizeof(RPSMessage));
        if (byteSent > sizeof(RPSMessage)) {
            warning("RPSMessage Request overflowed.");
        }

        switch (message.type) {
            case RPSMSG_SIGNUP :
                if (client1Tid == -1) {
                    client1Tid = tid;
                }
                else if (client2Tid == -1) {
                    client2Tid = tid;
                }
                else {
                    warning("Game full.");
                    break;
                }
                if (client2Tid != -1) {
                    Reply(client1Tid, "Game start.", 16);
                    Reply(client2Tid, "Game start.", 16);
                }
                break;
            case RPSMSG_PLAY :
                break;
            case RPSMSG_QUIT :
                if (tid == client1Tid) {
                    Reply(client1Tid, "Game Finish.", 16);
                    Reply(client2Tid, "Gamer Leave.", 16);
                }
                else if (tid == client2Tid) {
                    Reply(client2Tid, "Game Finish.", 16);
                    Reply(client1Tid, "Gamer Leave.", 16);
                }
                else {
                    warning("Not client quit.");
                }
                break;
            default :
                warning("Unknown RPSMessage Type.");
        }
    }

    Exit();
}

void rpsClient() {
    bwprintf(COM2, "---RPS Client Initialized---\n\r");
    int tid = WhoIs("RPS Server");

    RPSMessage message;
    char reply[16];

    message.type = RPSMSG_SIGNUP;
    Send(tid, &message, sizeof(RPSMessage), reply, 16);
    bwprintf(COM2, "Get reply: %s\n\r", reply);

    message.type = RPSMSG_PLAY;
    message.choice = RPSMSG_ROCK;
    Send(tid, &message, sizeof(RPSMessage), reply, 16);
    bwprintf(COM2, "Get reply: %s\n\r", reply);

    message.type = RPSMSG_QUIT;
    Send(tid, &message, sizeof(RPSMessage), reply, 16);
    bwprintf(COM2, "Get reply: %s\n\r", reply);
    Exit();
}

void firstUserTask() {
    int tid;
    bwprintf(COM2, "---Bootstrap---\n\r");

    // Create NameServer
    tid = Create(PRIORITY_HIGH, &nameServer);
    assertEquals(NAMESERVER_TID, tid, "NameServer should be the first task.");

    // Create RPS Server
    tid = Create(PRIORITY_MED + 2, &rpsServer);
    bwprintf(COM2, "RPS server tid = %d\n\r", tid);

    // Create RPS Client1
    tid = Create(PRIORITY_MED + 1, &rpsClient);
    bwprintf(COM2, "RPS client1 tid = %d\n\r", tid);

    // Create RPS Client2
    tid = Create(PRIORITY_MED + 1, &rpsClient);
    bwprintf(COM2, "RPS client2 tid = %d\n\r", tid);

    Exit();
}
