#<<<<<<< HEAD:common/tests/ping_pong/Makefile
include ../../makefile.gnu.config
#include ../../makefile.debug

#CFLAGS += -I../../phys_trans -I../../network -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall
#=======
FORCE32 ?= 1
CXX = /afs/csail/group/carbon/tools/openmpi/bin/mpicxx

ifeq (${FORCE32},1)
  CFLAGS = -I../../phys_trans -I../../network -I../../misc -I../../core -I../../../pin/src -I../../user -c -g -Wall -m32
  LDFLAGS = -m32 -m elf_i386
else
  CFLAGS = -I../../phys_trans -I../../network -I../../misc -I../../core -I../../../pin/src -I../../user -c -g -Wall
  LDFLAGS = 
endif

SOURCES = ../../user/capi.cc ping_pong.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: ping_pong

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<
#	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

ping_pong: $(OBJECTS)
	$(CXX) -pthread $(LDFLAGS) -o $@ $(OBJECTS)
#	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o ping_pong sim.out

squeaky: clean
	-rm -f *~
