#
# Makefile for kernel
#
XCC	= gcc
AS	= as
AR	= ar
CFLAGS  = -c -fPIC -Wall -I. -I../../include -mcpu=arm920t -msoft-float -fno-builtin
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

ARFLAGS = rcs

all:  kernel.a
	cp kernel.a ../../lib/libkernel.a

kernel.s: kernel.c 
	$(XCC) -S $(CFLAGS) kernel.c

kernel.o: kernel.s
	$(AS) $(ASFLAGS) -o kernel.o kernel.s

kernel.a: kernel.o
	$(AR) $(ARFLAGS) $@ kernel.o

clean:
	-rm -f *.s *.a *.o
