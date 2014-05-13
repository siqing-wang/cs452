 /*
 * kernel.c
 */

#include <bwio.h>

int main( int argc, char* argv[] ) {
    bwsetfifo(COM2, OFF);
    bwsetspeed(COM2, 115200);
    bwprintf(COM2, "Hello world");
    return 0;
}

