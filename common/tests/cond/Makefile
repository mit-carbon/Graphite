FORCE32 ?= 1

CC = /afs/csail/group/carbon/tools/openmpi/bin/mpicxx

ifeq (${FORCE32},1)
  CFLAGS = -I../../user -I../../phys_trans -c -g -Wall -m32
  LDFLAGS = -m32 -m elf_i386
else
  CFLAGS = -I../../user -I../../phys_trans -c -g -Wall
  LDFLAGS = 
endif

SOURCES = ../../user/capi.cc ../../user/mcp_api.cc ../../user/sync_api.cc cond_test.cc
OBJECTS = $(SOURCES:%.cc=%.o) 

## Build rules

all: cond_test

%.o : %.cc
	$(CC) -DDEBUG $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

cond_test: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning
clean:
	-rm -f *.o cond_test sim.out

squeaky: clean
	-rm -f *~


