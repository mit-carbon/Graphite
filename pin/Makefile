# this gives us default build rules and dependency handling
SIM_ROOT ?= $(CURDIR)/..

LD_LIBS += -lcarbon_sim

CLEAN=$(findstring clean,$(MAKECMDGOALS))

ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

# Use these files for auto targets
.SUFFIXES:  .o .c .h .cc

# Use the pin flags for building
include $(SIM_ROOT)/Makefile.config

TARGET = ia32e

override PIN_ROOT = $(PIN_HOME)
CONFIG_ROOT = $(PIN_HOME)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
include $(CONFIG_ROOT)/makefile.default.rules

CXXFLAGS += $(TOOL_CXXFLAGS)
CFLAGS += $(CXXFLAGS)

SOURCES = $(shell ls $(SIM_ROOT)/pin/*.cc $(SIM_ROOT)/pin/lite/*.cc)

OBJECTS = $(SOURCES:%.cc=%.o)

## build rules
TARGET = $(SIM_ROOT)/lib/pin_sim.so

all: $(TARGET)

$(SIM_ROOT)/lib/libcarbon_sim.a:
	$(MAKE) -C $(SIM_ROOT)/common

$(TARGET): $(PIN_HOME)/intel64/lib/libpin.a $(SIM_ROOT)/lib/libcarbon_sim.a
$(TARGET): $(OBJECTS)
	@echo $(TARGET)
	$(CXX) $(TOOL_LDFLAGS) $(LD_FLAGS) -o $@ $(OBJECTS) $(TOOL_LPATHS) $(TOOL_LIBS) $(LD_LIBS) $(DEBUG)

ifneq ($(CLEAN),)
clean:
	-rm -f $(TARGET) $(OBJECTS)
endif
