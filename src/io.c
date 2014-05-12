/* io.c - busy-wait I/O routines for diagnosis */

#include <io.h>

/* Output Buffers */
static char *ob2;
static char *ob1;
static int ob2_start;
static int ob1_start;
static int ob2_size;
static int ob1_size;

/* IO library initialization */

void ioinit(char* b1, char* b2) {
    ob1 = (char*)b1;
    ob1_start = 0; // posn of start
    ob1_size = 0;
    ob2 = (char*)b2;
    ob2_start = 0; // posn of start
    ob2_size = 0;
} 


/* Output Buffer Functions*/
int ob_empty(int channel) {
    switch( channel ) {
        case COM1:
            return ob1_size == 0;
        case COM2:
            return ob2_size == 0;
    }
    return -1;
}

int ob_full(int channel) {
    switch( channel ) {
        case COM1:
            return ob1_size == OB1_SPACE;
        case COM2:
            return ob2_size == OB2_SPACE;
    }
    return -1;
}

void ob_push(int channel, char c) {
    if (ob_full(channel)) {
        bwprintf(COM2, "[COM%d push fail!(full)\n]", channel);
        return;
    }

    int index;
    switch( channel ) {
        case COM1:
            index = (ob1_start + ob1_size) % OB1_SPACE;
            ob1_size++;
            *(ob1 + index) = c;
            break;
        case COM2:
            index = (ob2_start + ob2_size) % OB2_SPACE;
            ob2_size++;
            *(ob2 + index) = c;
            break;
        default:
            break;
    }
}

char ob_pop(int channel) {
    if (ob_empty(channel)) {
        bwprintf(COM2, "[COM%d pop fail!(empty)\n]", channel);
        return -1;
    }

    char c;
    switch( channel ) {
        case COM1:
            ob1_size--;
            c = (char) ob1[ob1_start];    
            ob1_start = (ob1_start + 1) % OB1_SPACE;
            return c;
        case COM2:
            ob2_size--;
            c = (char) ob2[ob2_start];    
            ob2_start = (ob2_start + 1) % OB2_SPACE;
            return c;
        default:
            break;
    }

    return -1; 
}

int ob_print(int channel) {
    if (ob_empty(channel)) {
        return 0;
    }
    int *flags, *data;
    switch( channel ) {
    case COM1:
        flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
        data = (int *)( UART1_BASE + UART_DATA_OFFSET );
        break;
    case COM2:
        flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
        data = (int *)( UART2_BASE + UART_DATA_OFFSET );
        break;
    default:
        return -1;
    }

    while (!( *flags & TXFF_MASK ) && !ob_empty(channel)) {
        if ((channel == COM1) && !(*flags & CTS_MASK)) {
            break;
        } else if ((channel == COM2) && (*flags & CTS_MASK)) {
            break;
        } else if ((channel == COM1) && (*flags & TXBUSY_MASK)) {
            break;
        }

        char c = (char) ob_pop(channel);
        *data = c;
    }
    return 1;
}

/* IO stuff */
int getc(int channel) {
    int *flags, *data;
    unsigned char c;

    switch( channel ) {
    case COM1:
        flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
        data = (int *)( UART1_BASE + UART_DATA_OFFSET );
        break;
    case COM2:
        flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
        data = (int *)( UART2_BASE + UART_DATA_OFFSET );
        break;
    default:
        return -1;
        break;
    }

    if ( !( *flags & RXFF_MASK ) ) {
        // No input available
        return -1;
    }
    c = *data;
    return c;
}

void putc( int channel, char c ) {
    ob_push(channel, c);
}

void putstr( int channel, char *str ) {
    while( *str ) {
        putc( channel, *str );
        str++;
    }
}

void putw( int channel, int n, char fc, char *bf ) {
    char ch;
    char *p = bf;

    while( *p++ && n > 0 ) n--;
    while( n-- > 0 ) putc( channel, fc );
    while( ( ch = *bf++ ) ) putc( channel, ch );
}


void format( int channel, char *fmt, va_list va ) {
    char bf[12];
    char ch, lz;
    int w;

    while ( ( ch = *(fmt++) ) ) {
        if ( ch != '%' )
            putc( channel, ch );
        else {
            lz = 0; w = 0;
            ch = *(fmt++);
            switch ( ch ) {
            case '0':
                lz = 1; ch = *(fmt++);
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                ch = a2i( ch, &fmt, 10, &w );
                break;
            }
            switch( ch ) {
            case 0: return;
            case 'c':
                putc( channel, va_arg( va, char ) );
                break;
            case 's':
                putw( channel, w, 0, va_arg( va, char* ) );
                break;
            case 'u':
                ui2a( va_arg( va, unsigned int ), 10, bf );
                putw( channel, w, lz, bf );
                break;
            case 'd':
                i2a( va_arg( va, int ), bf );
                putw( channel, w, lz, bf );
                break;
            case 'x':
                ui2a( va_arg( va, unsigned int ), 16, bf );
                putw( channel, w, lz, bf );
                break;
            case '%':
                putc( channel, ch );
                break;
            }
        }
    }
}

void printf( int channel, char *fmt, ... ) {
        va_list va;

        va_start(va,fmt);
        format( channel, fmt, va );
        va_end(va);
}
