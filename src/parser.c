/* parser.c - Parser and Parser helper function */

#include <parser.h>

/* Parser Helper */

int a2d( char ch ) {
    if( ch >= '0' && ch <= '9' ) return ch - '0';
    if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    return -1;
}

char a2i( char ch, char **src, int base, int *nump ) {
    int num, digit;
    char *p;

    p = *src; num = 0;
    while( ( digit = a2d( ch ) ) >= 0 ) {
        if ( digit > base ) break;
        num = num*base + digit;
        ch = *p++;
    }
    *src = p; *nump = num;
    return ch;
}

void ui2a( unsigned int num, unsigned int base, char *bf ) {
    int n = 0;
    int dgt;
    unsigned int d = 1;
    
    while( (num / d) >= base ) d *= base;
    while( d != 0 ) {
        dgt = num / d;
        num %= d;
        d /= base;
        if( n || dgt > 0 || d == 0 ) {
            *bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
            ++n;
        }
    }
    *bf = 0;
}

void i2a( int num, char *bf ) {
    if( num < 0 ) {
        num = -num;
        *bf++ = '-';
    }
    ui2a( num, 10, bf );
}

int readNum(char** input) {
    char* cur = *input;
    if (a2d(*cur) == -1) {
        /* invalid number */
        return -1;
    }

    int base = 10;
    if (stringStartWith(cur, "0x")) {
        base = 16; 
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

void readToken( char** orig, char* result ) {
    int i;
    result[0] = '\0';
    for (i = 0; ;i++) {
        char* str = *orig;
        if (*str == '\0') {
            result[i] = '\0';
            break;
        }
        if (*str == ' ') {
            result[i] = '\0';
            *orig = (*orig) + 1; 
            break;
        }
        result[i] = *str;
        *orig = (*orig) + 1; 
    } 
}

int readStrictToken(char** input, char* token) {
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

