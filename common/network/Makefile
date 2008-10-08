include ../makefile.gnu.config
include ../makefile.debug

DBG=-g
OPT=-O2
CFLAGS+=-c -I../../pin/src -I../user -I../core -I../phys_trans -I../misc -I../ -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = network.cc network_mesh_analytical.cc
OBJECTS = $(SOURCES:%.cc=%.o)


all: $(OBJECTS)


## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo
squeaky: clean
	-rm -f *~
