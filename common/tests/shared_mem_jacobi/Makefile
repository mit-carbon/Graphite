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

SOURCES = ../../user/capi.cc jacobi.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: jacobi

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

jacobi: $(OBJECTS)
	$(CXX) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o jacobi sim.out

squeaky: clean
	-rm -f *~
