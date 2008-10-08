.PHONY: use-mpi use-sm pick-version
include ../makefile.gnu.config

## Use this option to select the version of the PT layer you want to use ##
## Choices are: mpi, sm
PT_VERSION ?= sm

DBG=-g
OPT=-O2
CFLAGS=-c -I../misc -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = transport.cc
OBJECTS = $(SOURCES:%.cc=%.o)


all: $(OBJECTS)

use-mpi:
	ln -f -s mpitransport.h transport.h
	ln -f -s mpitransport.cc transport.cc

use-sm:
	ln -f -s smtransport.h transport.h
	ln -f -s smtransport.cc transport.cc


## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo
squeaky: clean
	-rm -f *~
