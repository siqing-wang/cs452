/*
 * user_tasks.h
 */

#ifndef __USER_TASKS_H__
#define __USER_TASKS_H__

#define RPSMSG_SIGNUP 0
#define RPSMSG_PLAY 1
#define RPSMSG_QUIT 2
#define RPSMSG_ROCK 0
#define RPSMSG_PAPER 1
#define RPSMSG_SCISSORS 2

/* Used by rock paper sessiors server/client. */
typedef struct RPSMessage
{
    int type;
    int choice;

} RPSMessage;

void firstUserTask();

#endif
