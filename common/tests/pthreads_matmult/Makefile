CC = /usr/bin/g++
CFLAGS = -I../../user -c -g -Wall
SOURCES = ../../user/capi.cc cannon.cc
OBJECTS = $(SOURCES:%.cc=%.o)

## Build rules

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

cannon: $(OBJECTS)
	$(CC) -pthread -o $@ $(OBJECTS)

all: cannon

## Cleaning

clean:
	-rm -f *.o cannon sim.out

squeaky: clean
	-rm -f *~
