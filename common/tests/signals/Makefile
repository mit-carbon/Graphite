CC = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall
SOURCES = ../../user/capi.cc signals.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

all: signals

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

signals: $(OBJECTS)
	$(CC) -pthread -o $@ $(OBJECTS)

## Cleaning

clean:
	-rm -f *.o signals sim.out

squeaky: clean
	-rm -f *~
