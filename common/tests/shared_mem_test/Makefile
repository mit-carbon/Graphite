include ../../makefile.gnu.config
include ../../makefile.debug

CC = /usr/bin/g++
CFLAGS += -I../../phys_trans -I../../network -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall
SOURCES = ../../user/capi.cc test.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: test

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

test: $(OBJECTS)
	$(CC) -pthread -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o test sim.out

squeaky: clean
	-rm -f *~
