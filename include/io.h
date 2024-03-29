/*
 * io.h
 *
 */

#ifndef __IO_H__
#define __IO_H__

void io_init(int channel);
void io_interrupt_enable(int channel, int mask);
void io_interrupt_disable(int channel, int type);
char io_getdata(int channel);
void io_putdata(int channel, char ch);

#endif
