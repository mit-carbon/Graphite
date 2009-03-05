
# this gives us default build rules and dependency handling
SIM_ROOT = $(PWD)
include $(SIM_ROOT)/common/makefile.common

DBG=-g
OPT=-O2
BUILD_32 = 1

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

# Use the pin flags for building
include ../../common/makefile.gnu.config
CXXFLAGS+=$(PIN_CXXFLAGS)
CFLAGS+=$(CXXFLAGS)

LD_LIBS += -pthread -lcarbon_sim

SOURCES = pin_sim.cc analysis.cc routine_replace.cc run_models.cc  \
          user_space_wrappers.cc pin_thread.cc

OBJECTS = $(SOURCES:%.cc=%.o)

## build rules
BUILD_TARGET = ../bin/pin_sim.so

../bin:
	-mkdir ../bin

all: $(BUILD_TARGET) ../bin

$(BUILD_TARGET): $(PIN_LIBNAMES)
$(BUILD_TARGET): $(OBJECTS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $^ -L$(OS_SERVICES_ROOT)/ia32 $(PIN_LIBS) $(LD_LIBS) $(DBG)

