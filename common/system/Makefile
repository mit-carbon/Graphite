.PHONY: use-mpi use-sm pick-version
include ../makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS=-c -I../core -I../system -I../network -I../misc -I../phys_trans -I../user -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = syscall_server.cc mcp.cc sync_client.cc sync_server.cc
OBJECTS = $(SOURCES:%.cc=%.o)


all: $(OBJECTS)

## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo

squeaky: clean
	-rm -f *~


