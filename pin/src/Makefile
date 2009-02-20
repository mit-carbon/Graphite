include ../../common/makefile.gnu.config
include ../../common/makefile.debug

DBG=-g
OPT=-O2

CXXFLAGS+=-c -I../../common/user -I../../common/system -I../../common/core -I../../common/network -I../../common/phys_trans -I../../common/misc -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

LDFLAGS= -pthread

CXXFLAGS+=-I../../os-services-linux-ia32/include
LDFLAGS+= -los-services -L ../../os-services-linux-ia32

# Uncomment the below for the 64bit version of os services
# CXXFLAGS+=-I../../os-services-linux-intel64/include
# LDFLAGS+= -los-services -L ../../os-services-linux-intel64

TOOL_ROOTS = ../bin/pin_sim
SOURCES = pin_sim.cc analysis.cc routine_replace.cc run_models.cc pin_lock.cc user_space_wrappers.cc pin_thread.cc
SRCOBJS = ../../common/user/*.o ../../common/core/*.o ../../common/network/*.o ../../common/phys_trans/*.o ../../common/misc/*.o ../../common/system/*.o
OBJECTS = $(SOURCES:%.cc=%.o) $(SRCOBJS)

## build rules
TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

# all is defined in the common makefile
all: ../bin $(TOOLS)

# this gives us default build rules and dependency handling
include ../../common/makefile.common

../bin:
	-mkdir ../bin

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): $(OBJECTS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $^  $(PIN_LIBS) -los-services $(DBG)

