/* parser.c - Parser and Parser helper function */

#include <parser.h>
#include <utils.h>
#include <ui.h>
#include <syscall.h>
#include <track.h>
#include <train_control.h>

/* Parser Helper */
int readNum(char** input);
int isWhiteSpace(char c);

/* Parser */
int skipWhiteSpace(char** input);
int readToken(char** input, char* token);

/* Commands */
int parseInitCommand(int trainCtrlTid, char* input);
int parseSetSpeedCommand(int trainCtrlTid, char* input);
int parseReverseDirectionCommand(int trainCtrlTid, char* input);
int parseTurnSwitchCommand(int trainCtrlTid, char* input);
int parseGoCommand(int trainCtrlTid, char* input);
int parseStopCommand(int trainCtrlTid, char* input);
int parseHaltCommand(int trainCtrlTid, char* input);
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

int readString( char** input, char* result ) {
    char* cur = *input;

    int i = 0;
    for (; ;i++) {
        if (*cur == '\0') {
            break;
        }
        if (*cur == ' ') {
            break;
        }
        result[i] = *cur;
        cur ++;
    }
    result[i] = '\0';

    if (*cur != '\0' && !isWhiteSpace(*cur)) {
        /* Has to be a whitespace (or end) after token */
        return 0;
    }
    skipWhiteSpace(&cur);
    *input = cur;
    return 1;
}

int parseCommand(int trainCtrlTid, char* input) {

    switch (input[0]) {
        case 'g':
            return parseGoCommand(trainCtrlTid, input);
        case 'i':
            return parseInitCommand(trainCtrlTid, input);
        case 'p':
            return parsePerformanceMonitor(input);
        case 'q':
            return parseHaltCommand(trainCtrlTid, input);
        case 'r':
            return parseReverseDirectionCommand(trainCtrlTid, input);
        case 's':
            return parseTurnSwitchCommand(trainCtrlTid, input);
        case 't':
            return parseSetSpeedCommand(trainCtrlTid, input);
        default:
            return CMD_FAILED;
    }
    return CMD_FAILED;
}


/* Parse commands */
int parseInitCommand(int trainCtrlTid, char* input){

    int train_number;

    /* read tr */
    if(!readToken(&input, "init")) {
        return CMD_FAILED;
    }

    /* read train number */
    train_number = readNum(&input);
    if(train_number < 0 || train_number > 80) {
        return CMD_FAILED;
    }

    /* read stop location */
    char location[6];
    if(!readString(&input, location)) {
        return CMD_FAILED;
    }

    /* Capitalize */
    int i;
    for (i = 0; i < 6; i++) {
        if ((location[i] >= 'a') && (location[i] <= 'z')) {
            location[i] = location[i] - 'a' + 'A';
        }
    }

    // Only accept A10, A8, A5
    if ((!stringEquals(location, "A10")) &&
        (!stringEquals(location, "A8")) &&
        (!stringEquals(location, "A5"))) {
        return CMD_FAILED;
    }

    PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sInitialize train %u at location %s %s", TCS_GREEN, train_number, location, TCS_RESET);

    TrainControlMessage message;
    message.type = TRAINCTRL_TR_INIT;
    message.num = train_number;
    for(i = 0; i < 6; i++) {
        message.location[i] = location[i];
    }
    int msg = 0;
    Send(trainCtrlTid, &message, sizeof(message), &msg, sizeof(msg));

    return CMD_SUCCEED;
}

int parseSetSpeedCommand(int trainCtrlTid, char* input){

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

    PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sSet speed of train %u to %u.%s", TCS_GREEN, train_number, train_speed, TCS_RESET);

    TrainControlMessage message;
    message.type = TRAINCTRL_TR_SETSPEED;
    message.num = train_number;
    message.data = train_speed;
    int msg = 0;
    Send(trainCtrlTid, &message, sizeof(message), &msg, sizeof(msg));

    return CMD_SUCCEED;
}

int parseReverseDirectionCommand(int trainCtrlTid, char* input) {

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

    PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sReverse train %u%s", TCS_GREEN, train_number, TCS_RESET);

    TrainControlMessage message;
    message.type = TRAINCTRL_TR_REVERSE;
    message.num = train_number;
    int msg = 0;
    Send(trainCtrlTid, &message, sizeof(message), &msg, sizeof(msg));

    return CMD_SUCCEED;
}

int parseTurnSwitchCommand(int trainCtrlTid, char* input) {

    int switch_number;
    int switch_direction;

    /* read sw */
    if(!readToken(&input, "sw")) {
        return CMD_FAILED;
    }

    /* read switch number */
    switch_number = readNum(&input);
    if(switch_number <= 0) {
        return CMD_FAILED;
    }
    if ((switch_number > 18) && (switch_number < 153)) {
        return CMD_FAILED;
    }
    if ((switch_number > 156)) {
        return CMD_FAILED;
    }

    /* read switch direction */
    if(readToken(&input, "S") || readToken(&input, "s")) {
        switch_direction = DIR_STRAIGHT;
    } else if (readToken(&input, "C") || readToken(&input, "c")) {
        switch_direction = DIR_CURVED;
    } else {
        return CMD_FAILED;
    }

    if (switch_direction == DIR_STRAIGHT) {
        PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sTurn switch %u straight", TCS_GREEN, switch_number);
    } else {
        PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sTurn switch %u curve", TCS_GREEN, switch_number);
    }
    PutStr(COM2, TCS_RESET);

    TrainControlMessage message;
    message.type = TRAINCTRL_SW_CHANGE;
    message.num = switch_number;
    message.data = switch_direction;
    int msg = 0;
    Send(trainCtrlTid, &message, sizeof(message), &msg, sizeof(msg));

    return CMD_SUCCEED;
}

int parseGoCommand(int trainCtrlTid, char* input) {
    /* read stop */
    if(!readToken(&input, "go")) {
        return CMD_FAILED;
    }

    /* read train number */
    int train_number = readNum(&input);
    if(train_number < 0) {
        return CMD_FAILED;
    }

    /* read stop location */
    char location[6];
    if(!readString(&input, location)) {
        return CMD_FAILED;
    }

    /* Capitalize */
    int i;
    for (i = 0; i < 6; i++) {
        if ((location[i] >= 'a') && (location[i] <= 'z')) {
            location[i] = location[i] - 'a' + 'A';
        }
    }

    /* read location offset */
    int offset = 0;
    if(readToken(&input, "+")) {
        offset = readNum(&input);
    }

    TrainControlMessage message;
    message.type = TRAINCTRL_TR_GO;
    message.num = train_number;
    for(i = 0; i < 6; i++) {
        message.location[i] = location[i];
    }
    message.data = offset * 10; // cm -> mm
    int msg = 0;
    Send(trainCtrlTid, &message, sizeof(message), &msg, sizeof(msg));

    PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sStop train %d at %s with offset %d%s", TCS_GREEN, train_number, location, offset, TCS_RESET);

    return CMD_SUCCEED;
}

int parseHaltCommand(int trainCtrlTid, char* input) {
    /* read sw */
    if(!readToken(&input, "q")) {
        return CMD_FAILED;
    }
    PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sProgram terminated\n\r%s", TCS_RED, TCS_RESET);

    TrainControlMessage message;
    message.type = TRAINCTRL_HALT;
    int msg = 0;
    Send(trainCtrlTid, &message, sizeof(message), &msg, sizeof(msg));

    return CMD_HALT;
}

int parsePerformanceMonitor(char* input) {
    /* read sw */
    if(!readToken(&input, "pm")) {
        return CMD_FAILED;
    }

    if (readToken(&input, "on")) {
        PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sTurn ON performance monitor\n\r%s", TCS_GREEN, TCS_RESET);
        return CMD_PM_ON;
    } else if (readToken(&input, "off")) {
        PrintfAt(COM2, LOG_R + 1, LOG_C + 4, "%sTurn OFF performance monitor\n\r%s", TCS_GREEN, TCS_RESET);
        return CMD_PM_OFF;
    } else {
        return CMD_FAILED;
    }
    return CMD_HALT;
}
