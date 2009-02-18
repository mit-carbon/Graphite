include ../../../common/makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS= -c -I./ -Wall -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

CFLAGS += -I../../../os-services-linux-ia32/include -m32
LDFLAGS= -L../../../os-services-linux-ia32 -los-services -m32
#CFLAGS += -I/afs/csail.mit.edu/u/c/cg3/src/pinthread_test/gruenwald/os-services-linux-ia32/include -m32
#LDFLAGS= -L/afs/csail.mit.edu/u/c/cg3/src/pinthread_test/gruenwald/os-services-linux-ia32 -los-services -m32
#CFLAGS += -I../../../os-services-linux-intel64/include
#LDFLAGS= -L../../../os-services-linux-intel64 -los-services

CXXFLAGS = $(CFLAGS)

TOOL_ROOTS = ./pin_sim
SOURCES = pin_sim.cc thread_test.cc
SRCOBJS = 
OBJECTS = $(SOURCES:%.cc=%.o)

TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

all: tools test
tools: $(TOOLS) $(SOURCES)


## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): $(OBJECTS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(SRCOBJS) $(PIN_LIBS) $(DBG)


test:
	$(CXX) -o test -m32 test.cc

clean:
	-rm -f *.o $(TOOLS) *.d *.rpo test

squeaky: clean
	-rm -f *~
