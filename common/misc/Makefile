include ../makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS=-c -I../network -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = utils.cc config.cc bit_vector.cc packetize.cc basic_hash.cc lockfree_hash.cc locked_hash.cc debug.cc semaphore.cc cond.cc log.cc
OBJECTS = $(SOURCES:%.cc=%.o)

all: $(OBJECTS)

## build rules

%.o : %.cc %.h
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo
squeaky: clean
	-rm -f *~
