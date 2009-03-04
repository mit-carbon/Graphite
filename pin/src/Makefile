
# this gives us default build rules and dependency handling
include ../../common/makefile.common

DBG=-g
OPT=-O2
BUILD_32 = 1

OS_SERVICES_ROOT = ../../os-services-25032-gcc.4.0.0-linux-ia32_intel64
BOOST_ROOT = /afs/csail/group/carbon/tools/boost_1_38_0

# Grab all c/c++ files from subdirs
# SIM_C_SOURCES = ../../common/user/*.c
SIM_CPP_SOURCES =                        \
          ../../common/core/*.cc        \
          ../../common/network/*.cc     \
          ../../common/transport/*.cc   \
          ../../common/misc/*.cc        \
          ../../common/config/*.cpp     \
          ../../common/system/*.cc

# Grab the actual filenames and convert them into .o files for dependencies
SIM_SOURCES = $(shell ls $(SIM_CPP_SOURCES) $(SIM_C_SOURCES))

# Convert the .c and .cc's in to .o's
SIM_OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SIM_SOURCES) ) ) )

# Use these files for auto targets
.SUFFIXES:  .o .c .h .cc

# Add include paths for the system and other CXX Flags
CXXFLAGS+=-c -I../../common/user        \
             -I../../common/system      \
             -I../../common/core        \
             -I../../common/transport   \
             -I../../common/network     \
             -I../../common/misc        \
             -I../../common/config      \
             -I./ -fomit-frame-pointer  \
             -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

# Add support for os services
LD_LIBS += -los-services
ifeq ($(BUILD_32),1)
  CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-ia32
  LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/ia32 -m32
else
  CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-intel64
  LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/intel64
endif

# Use the pin flags for building
include ../../common/makefile.gnu.config
CXXFLAGS+=$(PIN_CXXFLAGS)
CFLAGS=$(CXXFLAGS)

# Boost support
CXXFLAGS += -I$(BOOST_ROOT)
BOOST_SUFFIX = gcc41-mt-1_38
LDFLAGS += -L$(BOOST_ROOT)/stage_static -L../../lib
LD_LIBS += -lboost_filesystem-$(BOOST_SUFFIX) -lboost_system-$(BOOST_SUFFIX)

LD_LIBS += -pthread

TOOL_ROOTS = ../bin/pin_sim

SOURCES = pin_sim.cc analysis.cc routine_replace.cc run_models.cc  \
          user_space_wrappers.cc pin_thread.cc

OBJECTS = $(SOURCES:%.cc=%.o) $(SIM_OBJECTS)

## build rules
TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

# all is defined in the common makefile
all: ../bin $(TOOLS)

../bin:
	-mkdir ../bin

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): $(OBJECTS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $^ -L$(OS_SERVICES_ROOT)/ia32 $(PIN_LIBS) $(LD_LIBS) $(DBG)

