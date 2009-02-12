include ../../common/makefile.gnu.config
include ../../common/makefile.debug

DBG=-g
OPT=-O2

CFLAGS+=-c -I../../common/user -I../../common/system -I../../common/core -I../../common/network -I../../common/phys_trans -I../../common/misc -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

LDFLAGS= -pthread

CFLAGS+=-I../../os-services-linux-ia32/include
LDFLAGS+= -los-services -L ../../os-services-linux-ia32

# Uncomment the below for the 64bit version of os services
# CFLAGS+=-I../../os-services-linux-intel64/include
# LDFLAGS+= -los-services -L ../../os-services-linux-intel64

TOOL_ROOTS = ../bin/pin_sim
SOURCES = pin_sim.cc mcp_runner.cc net_thread_runner.cc pin_lock.cc sim_thread.cc user_space_wrappers.cc
SRCOBJS = ../../common/user/*.o ../../common/core/*.o ../../common/network/*.o ../../common/phys_trans/*.o ../../common/misc/*.o ../../common/system/*.o
OBJECTS = $(SOURCES:%.cc=%.o)

TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

all: bindir commonsubsystem tools
tools: $(TOOLS) $(SOURCES)


## build rules
bindir:
	-mkdir -f ../bin

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
