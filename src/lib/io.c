 /*
  * io.c
  */

#include <io.h>
#include <ts7200.h>
#include <utils.h>

void io_setfifo(int channel, int state);
void io_setspeed(int channel, int speed);

void io_init(int channel) {
    io_interrupt_enable(channel, MSIEN_MASK | RIEN_MASK | TIEN_MASK);
    io_setfifo(channel, OFF);

    switch(channel) {
        case COM1:
            io_setspeed(COM1, 2400);
            break;
        case COM2:
            io_setspeed(COM2, 115200);
            break;
    }
}

void io_interrupt_enable(int channel, int mask) {
    int *line;
    switch(channel) {
        case COM1:
            line = (int *)(UART1_BASE + UART_CTLR_OFFSET);
            break;
        case COM2:
            line = (int *)(UART2_BASE + UART_CTLR_OFFSET);
            break;
    }
    *line |= mask;
}

void io_interrupt_disable(int channel, int mask) {
    int *line;
    switch(channel) {
        case COM1:
            line = (int *)(UART1_BASE + UART_CTLR_OFFSET);
                break;
        case COM2:
            line = (int *)(UART2_BASE + UART_CTLR_OFFSET);
            break;
    }
    *line = *line & ~mask;
}

char io_getdata(int channel) {
    int *line;
    switch(channel) {
        case COM1:
            line = (int *)(UART1_BASE + UART_DATA_OFFSET);
                break;
        case COM2:
            line = (int *)(UART2_BASE + UART_DATA_OFFSET);
            break;
    }
    return *line;
}

void io_setfifo(int channel, int state) {
    int *line, buf;
    switch(channel) {
        case COM1:
            line = (int *)(UART1_BASE + UART_LCRH_OFFSET);
            break;
        case COM2:
            line = (int *)(UART2_BASE + UART_LCRH_OFFSET);
            break;
    }
    buf = *line;
    buf = state ? buf | FEN_MASK : buf & ~FEN_MASK;
    *line = buf;
}

void io_setspeed(int channel, int speed) {
    int *high, *low;
    switch(channel) {
        case COM1:
            high = (int *)(UART1_BASE + UART_LCRM_OFFSET);
            low = (int *)(UART1_BASE + UART_LCRL_OFFSET);
            break;
        case COM2:
            high = (int *)(UART2_BASE + UART_LCRM_OFFSET);
            low = (int *)(UART2_BASE + UART_LCRL_OFFSET);
            break;
    }
    switch( speed ) {
        case 115200:
            *high = 0x0;
            *low = 0x3;
            break;
        case 2400:
            *high = 0x0;
            *low = 0xbf;
            break;
    }
}
