#
# Makefile for main
#
XCC	= gcc
AS	= as
LD	= ld
CFLAGS  = -c -fPIC -Wall -I. -Iinclude -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -Llib

CSOURCES = $(wildcard src/lib/*.c) $(wildcard src/tasks/*.c)  $(wildcard src/kernel/*.c)
ASMSOURCES = $(wildcard src/lib/*.asm) $(wildcard src/kernel/*.asm)
ASMFILES = $(CSOURCES:.c=.s)
OBJECTS = $(CSOURCES:.c=.o) $(ASMSOURCES:.asm=.o)
MAIN = main
EXEC = final2

all: clean $(ASMFILES) $(OBJECTS) $(EXEC).elf

$(MAIN).s: $(MAIN).c
	$(XCC) -S $(CFLAGS) $(MAIN).c

$(MAIN).o: $(MAIN).s
	$(AS) $(ASFLAGS) -o $(MAIN).o $(MAIN).s

$(EXEC).elf: $(MAIN).o $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ main.o $(OBJECTS) -lgcc
	chmod 777 $(EXEC).elf
	cp $(EXEC).elf /u/cs452/tftp/ARM/wsqzym/$(EXEC).elf

src/%.o: src/%.s
	$(AS) $(ASFLAGS) -o $@ $<

src/%.o: src/%.asm
	$(AS) $(ASFLAGS) -o $@ $<

src/%.s: src/%.c
	$(XCC) -S $(CFLAGS) -o $@ $<

clean:
	-rm -f main.elf *.s *.o main.map
	-rm -f $(OBJECTS)
	-rm -f $(ASMFILES)

