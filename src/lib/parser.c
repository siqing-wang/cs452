/* parser.c - Parser and Parser helper function */

#include <parser.h>
#include <utils.h>
#include <ui.h>
#include <syscall.h>
#include <trainset.h>

/* Parser Helper */
int readNum(char** input);
int isWhiteSpace(char c);

/* Parser */
int skipWhiteSpace(char** input);
int readToken(char** input, char* token);

/* Commands */
int parseSetSpeedCommand(TrainSetData *data, char* input);
int parseReverseDirectionCommand(TrainSetData *data, char* input);
int parseTurnSwitchCommand(TrainSetData *data, char* input) ;
int parseHaltCommand(char* input);
int parsePerformanceMonitor(char* input);

/* Parser Helper */
int readNum(char** input) {
    char* cur = *input;
    if (a2d(*cur) == -1) {
        /* First digit is invalid number */
        return -1;
    }

    int base = 10;
    if (stringStartWith(cur, "0x")) {
        base = 16;
        cur += 2;
    }

    int num = 0;
    int digit;
    for (;;) {
        digit = a2d(*cur);
        if (digit == -1) {
            break;
        }
        num = num * base + digit;
        cur++;
    }

    if (*cur != '\0' && !skipWhiteSpace(&cur)) {
        return -1;
    }

    *input = cur;
    return num;
}

int isWhiteSpace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
            return 1;
    }
    return 0;
}

/* Parser */

int skipWhiteSpace(char** input) {
    if (!isWhiteSpace(**input)) {
        return 0;
    }
    while (isWhiteSpace(**input)) {
        (*input)++;
    }
    return 1;
}

int readToken(char** input, char* token) {
    char* cur = *input;

    while (*token != '\0') {
        if (*token != *cur) {
            return 0;
        }
        token++;
        cur++;
    }

    if (*cur != '\0' && !isWhiteSpace(*cur)) {
        /* Has to be a whitespace (or end) after token */
        return 0;
    }
    skipWhiteSpace(&cur);
    *input = cur;
    return 1;
}

int parseCommand(TrainSetData *data, char* input) {

    switch (input[0]) {
        case 't':
            return parseSetSpeedCommand(data, input);
        case 'r':
            return parseReverseDirectionCommand(data, input);
        case 's':
            return parseTurnSwitchCommand(data, input);
        case 'q':
            return parseHaltCommand(input);
        case 'p':
            return parsePerformanceMonitor(input);
        default:
            return CMD_FAILED;
    }
    return CMD_FAILED;
}


/* Parse commands */
int parseSetSpeedCommand(TrainSetData *data, char* input){

    int train_number, train_speed;

    /* read tr */
    if(!readToken(&input, "tr")) {
        return CMD_FAILED;
    }

    /* read train number */
    train_number = readNum(&input);
    if(train_number < 0 || train_number > 80) {
        return CMD_FAILED;
    }

    /* read train speed */
    train_speed = readNum(&input);
    if(train_speed < 0) {
        return CMD_FAILED;
    }

    Printf(COM2, "%sSet speed of train %u to %u.%s", TCS_GREEN, train_number, train_speed, TCS_RESET);
    trainset_setSpeed(data, train_number, train_speed);

    return CMD_SUCCEED;
}

int parseReverseDirectionCommand(TrainSetData *data, char* input) {

    int train_number;

    /* read rv */
    if(!readToken(&input, "rv")) {
        return CMD_FAILED;
    }

    /* read train number */
    train_number = readNum(&input);
    if(train_number < 0) {
        return CMD_FAILED;
    }

    Printf(COM2, "%sReverse train %u%s", TCS_GREEN, train_number, TCS_RESET);
    trainset_reverse(data, train_number);
    return CMD_SUCCEED;
}

int parseTurnSwitchCommand(TrainSetData *data, char* input) {

    int switch_number;
    int switch_direction;

    /* read sw */
    if(!readToken(&input, "sw")) {
        return CMD_FAILED;
    }

    /* read switch number */
    switch_number = readNum(&input);
    if(switch_number < 0) {
        return CMD_FAILED;
    }

    /* read switch direction */
    if(readToken(&input, "S") || readToken(&input, "s")) {
        switch_direction = SWITCH_STRAIGHT;
    } else if (readToken(&input, "C") || readToken(&input, "c")) {
        switch_direction = SWITCH_CURVE;
    } else {
        return CMD_FAILED;
    }

    Printf(COM2, "%sTurn switch %u", TCS_GREEN, switch_number);
    if (switch_direction == SWITCH_STRAIGHT) {
        Printf(COM2, " straight");
    } else {
        Printf(COM2, " curve");

    }
    PutStr(COM2, TCS_RESET);

    trainset_turnSwitch(data, switch_number, switch_direction);
    updateSwitchTable(data, switch_number);

    return CMD_SUCCEED;
}

int parseHaltCommand(char* input) {
    /* read sw */
    if(!readToken(&input, "q")) {
        return CMD_FAILED;
    }
    Printf(COM2, "%sProgram terminated\n\r%s", TCS_RED, TCS_RESET);
    return CMD_HALT;
}

int parsePerformanceMonitor(char* input) {
    /* read sw */
    if(!readToken(&input, "pm")) {
        return CMD_FAILED;
    }

    if (readToken(&input, "on")) {
        Printf(COM2, "%sTurn ON performance monitor\n\r%s", TCS_GREEN, TCS_RESET);
        return CMD_PM_ON;
    } else if (readToken(&input, "off")) {
        Printf(COM2, "%sTurn OFF performance monitor\n\r%s", TCS_GREEN, TCS_RESET);
        return CMD_PM_OFF;
    } else {
        return CMD_FAILED;
    }
    return CMD_HALT;
}
