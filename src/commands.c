/* commands.c - Parse all commands */

#include <commands.h>

int parseCommand(char* input) {

    switch (input[0]) {
        case 't':
            return parseSetSpeedCommand(input);
        case 'r':
            return parseReverseDirectionCommand(input);
        case 's':
            return parseTurnSwitchCommand(input);
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
int parseSetSpeedCommand(char* input){ 

    int train_number, train_speed;

    /* read tr */
    if(!readStrictToken(&input, "tr")) {
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
    
    printf(COM2, "%sSet speed of train %u to %u.%s", TCS_GREEN, train_number, train_speed, TCS_RESET);
    trainset_setSpeed(train_number, train_speed);

    return CMD_SUCCEED;
}

int parseReverseDirectionCommand(char* input) {

    int train_number;

    /* read rv */
    if(!readStrictToken(&input, "rv")) {
        return CMD_FAILED;
    }

    /* read train number */
    train_number = readNum(&input);
    if(train_number < 0) {
        return CMD_FAILED;
    }

    printf(COM2, "%sReverse train %u%s", TCS_GREEN, train_number, TCS_RESET);
    trainset_reverse(train_number);
    return CMD_SUCCEED;
}

int parseTurnSwitchCommand(char* input) {

    int switch_number;
    int switch_direction;

    /* read sw */
    if(!readStrictToken(&input, "sw")) {
        return CMD_FAILED;
    }

    /* read switch number */
    switch_number = readNum(&input);
    if(switch_number < 0) {
        return CMD_FAILED;
    }

    /* read switch direction */
    if(readStrictToken(&input, "S") || readStrictToken(&input, "s")) {
        switch_direction = SWITCH_STRAIGHT;
    } else if (readStrictToken(&input, "C") || readStrictToken(&input, "c")) {
        switch_direction = SWITCH_CURVE;
    } else {
        return CMD_FAILED;
    }

    printf(COM2, "%sTurn switch %u", TCS_GREEN, switch_number);
    if (switch_direction == SWITCH_STRAIGHT) {
        printf(COM2, " straight");
    } else {
        printf(COM2, " curve");

    }
    putstr(COM2, TCS_RESET);

    trainset_turnSwitch(switch_number, switch_direction);
    updateSwitchTable(switch_number);

    return CMD_SUCCEED;
}

int parseHaltCommand(char* input) {
    /* read sw */
    if(!readStrictToken(&input, "q")) {
        return CMD_FAILED;
    }
    printf(COM2, "%sProgram terminated\n\r%s", TCS_RED, TCS_RESET);
    return CMD_HALT;
}

int parsePerformanceMonitor(char* input) {
    /* read sw */
    if(!readStrictToken(&input, "pm")) {
        return CMD_FAILED;
    }

    if (readStrictToken(&input, "on")) {
        printf(COM2, "%sTurn ON performance monitor\n\r%s", TCS_GREEN, TCS_RESET);
        return CMD_PM_ON;
    } else if (readStrictToken(&input, "off")) {
        printf(COM2, "%sTurn OFF performance monitor\n\r%s", TCS_GREEN, TCS_RESET);
        return CMD_PM_OFF;
    } else {
        return CMD_FAILED;
    }
    return CMD_HALT;
}
