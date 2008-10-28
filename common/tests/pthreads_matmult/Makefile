FORCE32 ?= 0

CC = /usr/bin/g++
ifeq (${FORCE32},1)
  CFLAGS = -I../../user -c -g -Wall -m32
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
