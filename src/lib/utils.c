/*
 *	utils.c
 */

#include <utils.h>
#include <bwio.h>
#include <ui.h>
#include <syscall.h>

void assert(int cond, char* msg) {
    if (!cond) {
        Log("%sASSERTION FAILED: %s%s", TCS_RED, msg, TCS_RESET);
    }
}

void assertEquals(int expected, int actual, char* msg) {
    if (expected != actual) {
        Log("%sASSERSION FAILED: %s; expected=%d, actual=%d%s", TCS_RED, msg,
            expected, actual, TCS_RESET);
    }
}

void debug(char *msg) {
    if (DEBUG_MODE) {
        bwputstr(COM2, msg);
        bwputstr(COM2, "\n\r");
    }
}

void warning(char *msg) {
    Log("%sWARNING: %s%s", TCS_YELLOW, msg, TCS_RESET);
}

void displayTime(unsigned int timerCount, int row, int col) {

    int tenthSecond = timerCount;
    int seconds = tenthSecond / 10;
    int minutes = seconds / 60;
    PrintfAt(COM2, row, col, "%u'%u.%u  ",
        minutes,
        seconds % 60,
        tenthSecond % 10);
}

void memcopy(char *dest, const char *src, int size) {
    int i;
    unsigned long *destll = (unsigned long*) dest;
    unsigned long *srcll = (unsigned long*) src;
    for (i = 0; i < size / (sizeof(unsigned long)); i++) {
        *destll = *srcll;
        destll++;
        srcll++;
    }
    dest = (char*)destll;
    src = (char*)srcll;
    for (i = 0; i < size % (sizeof(unsigned long)); i++) {
        *dest = *src;
        dest++;
        src++;
    }
}


int stringStartWith(char* s1, char* s2) {
    for(;;) {
        if (*s2 == '\0') {
            return 1;
        }
        else if (*s1 == '\0') {
            return 0;
        }
        else if (*s1 != *s2) {
            return 0;
        }
        else {
            s1 ++;
            s2 ++;
        }
    }
}

int stringLen(const char *s) {
    int len = 0;            // \0 not included in length
    while (*s != '\0') {
        len++;
        s = s+1;
        if (len > 500) {
            warning("stringLen: string length > 500, \\0 included? returning -1...");
            return -1;
        }
    }
    return len;
}

int stringEquals(const char* s1, char* s2) {
    for(;;) {
        if ((*s1 == '\0') && (*s2 == '\0')) {
            return 1;
        }
        else if (*s1 == '\0') {
            return 0;
        }
        else if (*s2 == '\0') {
            return 0;
        }
        else if (*s1 != *s2) {
            return 0;
        }
        else {
            s1 ++;
            s2 ++;
        }
    }
}

int a2d(char ch) {
    if( ch >= '0' && ch <= '9' ) return ch - '0';
    if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    return -1;
}

char a2i(char ch, char **src, int base, int *nump) {
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

void ui2a(unsigned int num, unsigned int base, char *bf) {
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

void i2a(int num, char *bf) {
    if( num < 0 ) {
        num = -num;
        *bf++ = '-';
    }
    ui2a( num, 10, bf );
}

int putwToBuffer(char *buf, int n, char fc, char *bf) {
    char ch;
    char *p = bf;
    int size = 0;

    while(*p++ && n > 0) {
        n--;
    }
    while(n-- > 0) {
        buf[size] = fc;
        size++;
    }
    while((ch = *bf++)) {
        buf[size] = ch;
        size++;
    }
    return size;
}


/* XoR shift random. */
unsigned long rand(unsigned long x) {
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    return x * 2685821657736338717LL;
}

int computeHash(const char *str) {
    int hash = 0;
    while(*str != '\0') {
        hash = (hash * 29 + (int)(*str)) % HASH_TABLE_SIZE;
        str++;
    }
    return hash;
}
