CC = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall -m32
LDFLAGS = -m32 -m elf_i386
SOURCES = ../../user/capi.cc file_io.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: file_io

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

file_io: $(OBJECTS)
	$(CC) -pthread $(LDFLAGS) -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o file_io sim.out

squeaky: clean
	-rm -f *~


