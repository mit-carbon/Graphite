include ../../makefile.gnu.config

FORCE32 ?= 1

# CC = /usr/bin/g++
CXX = /afs/csail/group/carbon/tools/openmpi/bin/mpicxx
TEST_NEW_SOURCES = ../../user/capi.c test_new.cc 
TEST_EVIC_SOURCES = ../../user/capi.c test_evic.cc
TEST_NEW_OBJECTS = $(TEST_NEW_SOURCES:%.cc=%.o)
TEST_EVIC_OBJECTS = $(TEST_EVIC_SOURCES:%.cc=%.o)

ifeq (${FORCE32},1)
	CFLAGS += -I../../transport -I../../network -I../../system -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall -m32
	LDFLAGS = -m32 -m elf_i386
else
	CFLAGS += -I../../transport -I../../network -I../../system -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall
	LDFLAGS =
endif

## Build rules

all: test_new test_evic

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

test_new: $(TEST_NEW_OBJECTS)
	$(CXX) -pthread $(LDFLAGS) -o $@ $(TEST_NEW_OBJECTS)

test_evic: $(TEST_EVIC_OBJECTS)
	$(CXX) -pthread $(LDFLAGS) -o $@ $(TEST_EVIC_OBJECTS)

## Cleaning

clean:
	-rm -f *.o test test_evic sim.out

squeaky: clean
	-rm -f *~
