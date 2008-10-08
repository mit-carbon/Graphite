#<<<<<<< HEAD:common/tests/ping_pong/Makefile
include ../../makefile.gnu.config
include ../../makefile.debug

CFLAGS += -I../../phys_trans -I../../network -I../../misc -I../../../pin/src -I../../core -I../../user -c -g -Wall
CC = /usr/bin/g++
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
#>>>>>>> master:common/tests/ping_pong/Makefile
SOURCES = ../../user/capi.cc ping_pong.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: ping_pong

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

ping_pong: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o ping_pong sim.out

squeaky: clean
	-rm -f *~
