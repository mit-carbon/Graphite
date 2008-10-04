include ../../makefile.gnu.config
include ../../makefile.debug

CC = /usr/bin/g++
CFLAGS += -I../../phys_trans -I../../network -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall
SOURCES = ../../user/capi.cc jacobi.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: jacobi

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

jacobi: $(OBJECTS)
	$(CC) -pthread -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o jacobi sim.out

squeaky: clean
	-rm -f *~
