/* util.c - utility library */

#include <util.h>

/* System */

void nop() {
    int i = 0;
    for (;i<500;i++);
}

void memcpy(char *dest, const char *src, int n) {
  int i = 0;
  for (; i < n ; i++) {
    *dest = *src;
    dest++;
    src++;
  }
}

void assert(int i) {
    /* Debug use only */
    if (!i) {
        bwputstr(COM2, "assertion failed");
    }
}

/* String */

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

int stringEqual(char* s1, char* s2) {
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

/* Math */

int getLogValue( int data, int base ) {
	if (data == 0) {
		return -1;
	}
	int count = 0;
	while (data != 1) {
		data = data / base;
		count ++;
	}
	return count;
}

int getExpValue( int data, int exp ) {
	int result = 1;
	int i;
	for(i = 0; i < exp; i++) {
		result = result * data;
	}
	return result;
}
