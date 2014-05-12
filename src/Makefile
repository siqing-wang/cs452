#
# Makefile for busy-wait IO library
#
XCC	= gcc
AS	= as
AR	= ar
CFLAGS  = -c -fPIC -Wall -I. -I../include -mcpu=arm920t -msoft-float -fno-builtin
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

ARFLAGS = rcs

all:  bwio.a util.a timer.a io.a ui.a parser.a trainset.a commands.a
	rm -rf ../lib
	mkdir ../lib
	cp bwio.a ../lib/libbwio.a
	cp util.a ../lib/libutil.a
	cp timer.a ../lib/libtimer.a
	cp io.a ../lib/libio.a
	cp ui.a ../lib/libui.a
	cp parser.a ../lib/libparser.a
	cp trainset.a ../lib/libtrainset.a
	cp commands.a ../lib/libcommands.a

# io
bwio.s: bwio.c 
	$(XCC) -S $(CFLAGS) bwio.c

bwio.o: bwio.s
	$(AS) $(ASFLAGS) -o bwio.o bwio.s

bwio.a: bwio.o
	$(AR) $(ARFLAGS) $@ bwio.o

# util
util.s: util.c 
	$(XCC) -S $(CFLAGS) util.c

util.o: util.s
	$(AS) $(ASFLAGS) -o util.o util.s

util.a: util.o
	$(AR) $(ARFLAGS) $@ util.o

# timer
timer.s: timer.c 
	$(XCC) -S $(CFLAGS) timer.c

timer.o: timer.s
	$(AS) $(ASFLAGS) -o timer.o timer.s

timer.a: timer.o
	$(AR) $(ARFLAGS) $@ timer.o

# io
io.s: io.c 
	$(XCC) -S $(CFLAGS) io.c

io.o: io.s
	$(AS) $(ASFLAGS) -o io.o io.s

io.a: io.o
	$(AR) $(ARFLAGS) $@ io.o

# ui
ui.s: ui.c 
	$(XCC) -S $(CFLAGS) ui.c

ui.o: ui.s
	$(AS) $(ASFLAGS) -o ui.o ui.s

ui.a: ui.o
	$(AR) $(ARFLAGS) $@ ui.o

# parser
parser.s: parser.c 
	$(XCC) -S $(CFLAGS) parser.c

parser.o: parser.s
	$(AS) $(ASFLAGS) -o parser.o parser.s

parser.a: parser.o
	$(AR) $(ARFLAGS) $@ parser.o

# trainset
trainset.s: trainset.c 
	$(XCC) -S $(CFLAGS) trainset.c

trainset.o: trainset.s
	$(AS) $(ASFLAGS) -o trainset.o trainset.s

trainset.a: trainset.o
	$(AR) $(ARFLAGS) $@ trainset.o

# commands
commands.s: commands.c 
	$(XCC) -S $(CFLAGS) commands.c

commands.o: commands.s
	$(AS) $(ASFLAGS) -o commands.o commands.s

commands.a: commands.o
	$(AR) $(ARFLAGS) $@ commands.o

clean:
	-rm -f *.s *.a *.o
