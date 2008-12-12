include ../../common/makefile.gnu.config
include ../../common/makefile.debug

DBG=-g
OPT=-O2

CFLAGS+=-c -I../../common/user -I../../common/system -I../../common/core -I../../common/network -I../../common/phys_trans -I../../common/misc -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
CFLAGS+=-I../../os-services-linux-ia32/include

LDFLAGS= -pthread -los-services -L ../../os-services-linux-ia32

TOOL_ROOTS = ../bin/pin_sim
SOURCES = chip.cc pin_sim.cc thread_test.cc
SRCOBJS = ../../common/user/*.o ../../common/core/*.o ../../common/network/*.o ../../common/phys_trans/*.o ../../common/misc/*.o ../../common/system/*.o
OBJECTS = $(SOURCES:%.cc=%.o)

TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

all: commonsubsystem tools
tools: $(TOOLS) $(SOURCES)


## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

commonsubsystem:
	$(MAKE) -C ../../common sim

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): $(OBJECTS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(SRCOBJS) $(PIN_LIBS) -los-services $(DBG)


clean:
	$(MAKE) -C ../../common clean
	-rm -f *.o $(TOOLS) *.d *.rpo

squeaky: clean
	$(MAKE) -C ../../common squeaky
	-rm -f *~
