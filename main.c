 /*
 * main.c
 */

#include <kernel.h>
#include <context_switch.h>
#include <bwio.h>

int main( int argc, char *argv[] ) {
    // declare kernel data structures
    // initialize( tds ); // tds is an array of TDs
    int i;
    for( i = 0; i < 4; i++ ) {

        // active = schedule( tds );
        TD* active;
        Request* req;
        kerxit( active, req ); // req is a pointer to a Request
        // handle( tds, req );
    }
    return 0;
}
