 /*
 * main.c
 */

#include <kernel.h>
#include <bwio.h>

int main( int argc, char *argv[] ) {
    kernel_run();
    bwprintf(COM2, "Finished\n\r");
    return 0;
}
