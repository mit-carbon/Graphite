include ../../makefile.gnu.config
include ../../makefile.debug

FORCE32 ?= 1

CC = /usr/bin/g++
SOURCES = ../../user/capi.cc test_1.cc
OBJECTS = $(SOURCES:%.cc=%.o)

ifeq (${FORCE32},1)
	CFLAGS += -I../../phys_trans -I../../network -I../../system -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall -m32
	LDFLAGS = -m32 -m elf_i386
else
	CFLAGS += -I../../phys_trans -I../../network -I../../system -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall
	LDFLAGS =
endif

## Build rules

all: test_1

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

test_1: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o test sim.out

squeaky: clean
	-rm -f *~
