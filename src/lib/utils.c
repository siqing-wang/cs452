/*
 *	utils.c
 */

#include <utils.h>
#include <bwio.h>


#define DEBUG_MODE 1

void assert(int cond, char* msg) {
	if (!cond) {
		bwputstr(COM2, "\033[31mAssertion Failed: ");
		bwputstr(COM2, msg);
		bwputstr(COM2, "\n\r\033[0m");
	}
}

void assertEquals(int expected, int actual, char* msg) {
	if (expected != actual) {
		bwputstr(COM2, "\033[31mAssertion Failed: ");
		bwputstr(COM2, msg);
		bwprintf(COM2, " expected = %d, actual = %d\n\r\033[0m", expected, actual);
	}
}

void debug(char *msg) {
	if (DEBUG_MODE) {
		bwputstr(COM2, msg);
		bwputstr(COM2, "\n\r");
	}
}

