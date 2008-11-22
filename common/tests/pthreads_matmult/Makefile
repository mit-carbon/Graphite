#for shared_mem, I added pin.h stuff into capi.cc, which means
# i need pin flags to find pin.H (TODO remove dependencies on pin.h from capi.cc)
include ../../makefile.gnu.config
#PIN_HOME = /afs/csail/group/carbon/tools/pin/current
#PIN_CXXFLAGS = -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -g -fno-strict-aliasing -I/afs/csail/group/carbon/tools/pin/current/Include -I/afs/csail/group/carbon/tools/pin/current/InstLib -I/afs/csail/group/carbon/tools/pin/current/extras/xed2-ia32/include -I/afs/csail/group/carbon/tools/pin/current/source/include -I/afs/csail/group/carbon/tools/pin/current/source/include/gen -DTARGET_IA32 -DTARGET_LINUX


FORCE32 ?= 1

CC = /usr/bin/gcc
CXX = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall -std=c99
LDFLAGS = -lm
ifeq (${FORCE32},1)
  CFLAGS += -m32
  LDFLAGS += -m32 -m elf_i386
endif
#C_SOURCES = ../../user/capi.c cannon.c
CPP_SOURCES = 
OBJECTS = $(CPP_SOURCES:%.cc=%.o) $(C_SOURCES:%.c=%.o) 

## Build rules

all: cannon

%.o : %.c
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

cannon: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o cannon sim.out

squeaky: clean
	-rm -f *~

out:
	echo $(PIN_CXXFLAGS)


