FORCE32 ?= 1

CC = /usr/bin/gcc
CXX = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall -std=c99
LDFLAGS = -lm
ifeq (${FORCE32},1)
  CFLAGS += -m32
  LDFLAGS += -m32 -m elf_i386
endif
C_SOURCES = ../../user/capi.c cannon.c
CPP_SOURCES = 
OBJECTS = $(CPP_SOURCES:%.cc=%.o) $(C_SOURCES:%.c=%.o) 

## Build rules

all: cannon

%.o : %.c
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

cannon: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o cannon sim.out

squeaky: clean
	-rm -f *~
