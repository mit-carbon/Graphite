CC = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall -m32
LDFLAGS = -m32 -m elf_i386
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
