// commands.h
// Author: Yu Meng Zhang
// Parse all commands

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <parser.h>
#include <io.h>
#include <trainset.h>

int parseCommand(char* input);
int parseSetSpeedCommand(char* input);
int parseReverseDirectionCommand(char* input);
int parseTurnSwitchCommand(char* input) ;
int parseHaltCommand(char* input);
int parsePerformanceMonitor(char* input);

#endif
