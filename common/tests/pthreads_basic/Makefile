include ../../makefile.gnu.config
include ../../makefile.debug

FORCE32 ?= 1
CXX = /afs/csail/group/carbon/tools/openmpi/bin/mpicxx

ifeq (${FORCE32},1)
  CFLAGS = -I../../phys_trans -I../../network -I../../misc -I../../core -I../../../pin/src -I../../user -c -g -Wall -m32
  LDFLAGS = -m32 -m elf_i386
else
  CFLAGS = -I../../phys_trans -I../../network -I../../misc -I../../core -I../../../pin/src -I../../user -c -g -Wall
  LDFLAGS = 
endif

SOURCES = ../../user/mcp_api.cc ../../user/capi.cc basic.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: basic 

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

basic: $(OBJECTS)
	$(CXX) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o basic sim.out

squeaky: clean
	-rm -f *~
