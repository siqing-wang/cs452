/*
 * io.h
 *
 */

#ifndef __IO_H__
#define __IO_H__

#define COM1    0
#define COM2    1

#define ON  1
#define OFF 0

void io_init(int channel);
void io_interrupt_enable(int channel, int mask);
void io_interrupt_disable(int channel, int type);
char io_getdata(int channel);

#endif
