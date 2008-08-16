CC = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall
SOURCES = ../../user/capi.cc basic.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: basic 

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

basic: $(OBJECTS)
	$(CC) -pthread -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o basic sim.out

squeaky: clean
	-rm -f *~
