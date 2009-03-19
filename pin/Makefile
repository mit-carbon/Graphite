# this gives us default build rules and dependency handling
SIM_ROOT ?= $(CURDIR)/..

LD_LIBS += -pthread -lcarbon_sim

CLEAN=$(findstring clean,$(MAKECMDGOALS))

ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

# Use these files for auto targets
.SUFFIXES:  .o .c .h .cc

# Add other CXX Flags
CXXFLAGS += -c -fomit-frame-pointer  \
            -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

# Use the pin flags for building
TARGET = ia32
PIN_HOME = /afs/csail/group/carbon/tools/pin/current/
include $(PIN_HOME)/source/tools/makefile.gnu.config

CXXFLAGS += $(PIN_CXXFLAGS)
CFLAGS += $(CXXFLAGS)

SOURCES = pin_sim.cc analysis.cc routine_replace.cc run_models.cc  \
          pin_thread.cc handle_args.o

OBJECTS = $(SOURCES:%.cc=%.o)

## build rules
TARGET = $(SIM_ROOT)/lib/pin_sim.so

all: $(TARGET)

$(SIM_ROOT)/lib/libcarbon_sim.a:
	$(MAKE) -C $(SIM_ROOT)/common

$(TARGET): $(PIN_LIBNAMES) $(SIM_ROOT)/lib/libcarbon_sim.a
$(TARGET): $(OBJECTS)
	@echo $(TARGET)
	$(CXX) $(PIN_LDFLAGS) $(LD_FLAGS) -o $@ $(OBJECTS) $(PIN_LIBS) $(LD_LIBS) $(DBG)

ifneq ($(CLEAN),)
clean:
	-rm -f $(TARGET) $(OBJECTS)
endif
