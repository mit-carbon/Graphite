include ../../../common/makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS=-c -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
CFLAGS += -I../../../os-services-linux-ia32/include
LDFLAGS= -L../../../os-services-linux-ia32 -los-services

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
	$(CXX) -o test test.cc

clean:
	-rm -f *.o $(TOOLS) *.d *.rpo test

squeaky: clean
	-rm -f *~
