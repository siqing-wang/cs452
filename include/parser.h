// parser.h
// Author: Yu Meng Zhang
// Parser and Parser helper function

#ifndef __PARSER_H__
#define __PARSER_H__

#define CMD_FAILED      0
#define CMD_SUCCEED     1
#define CMD_HALT        2
#define CMD_PM_ON       3
#define CMD_PM_OFF      4

#include <trainset.h>

/* Parse Commands. */
int parseCommand(TrainSetData *data, char* input);
#endif
