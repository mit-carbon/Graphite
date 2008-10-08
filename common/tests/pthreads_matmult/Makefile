#<<<<<<< HEAD:common/tests/pthreads_matmult/Makefile
include ../../makefile.gnu.config

CC = /usr/bin/g++
CFLAGS = -I../../user -I../../../pin/src -I../../core -I../../misc -c -g -Wall
#=======
FORCE32 ?= 1

CC += /usr/bin/g++
ifeq (${FORCE32},1)
  CFLAGS += -I../../user -c -g -Wall -m32
  LDFLAGS = -m32 -m elf_i386
else
  CFLAGS += -I../../user -c -g -Wall
  LDFLAGS = 
endif
#>>>>>>> master:common/tests/pthreads_matmult/Makefile
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
