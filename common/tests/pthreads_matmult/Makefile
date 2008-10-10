#for shared_mem, I added pin.h stuff into capi.cc, which means
# i need pin flags to find pin.H (TODO remove dependencies on pin.h from capi.cc)
include ../../makefile.gnu.config
#PIN_HOME = /afs/csail/group/carbon/tools/pin/current
#PIN_CXXFLAGS = -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -g -fno-strict-aliasing -I/afs/csail/group/carbon/tools/pin/current/Include -I/afs/csail/group/carbon/tools/pin/current/InstLib -I/afs/csail/group/carbon/tools/pin/current/extras/xed2-ia32/include -I/afs/csail/group/carbon/tools/pin/current/source/include -I/afs/csail/group/carbon/tools/pin/current/source/include/gen -DTARGET_IA32 -DTARGET_LINUX


FORCE32 ?= 1

#	CFLAGS = -I../../user -c -g -Wall -m32
CC = /usr/bin/g++
ifeq (${FORCE32},1)
	CFLAGS = -I../../user -I../../core -I../../misc -I../../../pin/src -c -g -Wall -m32
	LDFLAGS = -m32 -m elf_i386
else
	CFLAGS = -I../../user -c -g -Wall
	LDFLAGS =
endif
SOURCES = ../../user/capi.cc cannon.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: cannon

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

cannon: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o cannon sim.out

squeaky: clean
	-rm -f *~

out:
	echo $(PIN_CXXFLAGS)

#<<<<<<< HEAD:common/tests/pthreads_matmult/Makefile
#include ../../makefile.gnu.config

#CFLAGS = -I../../user -I../../../pin/src -I../../core -I../../misc -c -g -Wall
#=======

#CC = /usr/bin/g++
#ifeq (${FORCE32},1)
#  CFLAGS = -I../../user -I../../core -I../../misc -I../../../pin/src -c -g -Wall -m32
#  LDFLAGS = -m32 -m elf_i386
#else
#  CFLAGS = -I../../user -c -g -Wall
#  LDFLAGS = 
#endif
#>>>>>>> master:common/tests/pthreads_matmult/Makefile
#SOURCES = ../../user/capi.cc cannon.cc
#OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

#all: cannon

#%.o : %.cc
#	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<
#	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

#cannon: $(OBJECTS)
#	$(CXX) -pthread $(LDFLAGS) -o $@ $(OBJECTS)
#	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

#clean:
#	-rm -f *.o cannon sim.out

#squeaky: clean
#	-rm -f *~
