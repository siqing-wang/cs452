#
# Makefile for busy-wait IO tests
#
XCC	= gcc
AS	= as
LD	= ld
CFLAGS  = -c -fPIC -Wall -I. -I../../include -mcpu=arm920t -msoft-float -fno-builtin 
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map kernel.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L../../lib

all:  kernel.s kernel.elf

kernel.s: kernel.c
	$(XCC) -S $(CFLAGS) kernel.c

kernel.o: kernel.s
	$(AS) $(ASFLAGS) -o kernel.o kernel.s

kernel.elf: kernel.o
	$(LD) $(LDFLAGS) -o $@ kernel.o -lbwio -lgcc

clean:
	-rm -f kernel.elf *.s *.o kernel.map
