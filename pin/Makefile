# this gives us default build rules and dependency handling
SIM_ROOT ?= $(CURDIR)/..

CLEAN=$(findstring clean,$(MAKECMDGOALS))

ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
include $(SIM_ROOT)/contrib/Makefile.common
endif

# Use the pin flags for building
include $(SIM_ROOT)/Makefile.config

TARGET = ia32e

override PIN_ROOT = $(PIN_HOME)
CONFIG_ROOT = $(PIN_HOME)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
include $(CONFIG_ROOT)/makefile.default.rules

CXXFLAGS += $(TOOL_CXXFLAGS)
CFLAGS += $(CXXFLAGS)

# Math, pthread libraries
LD_LIBS += -lm -pthread

SOURCES = $(shell ls $(SIM_ROOT)/pin/*.cc $(SIM_ROOT)/pin/lite/*.cc)

OBJECTS = $(SOURCES:%.cc=%.o)
DEPENDENCIES = $(SOURCES:%.cc=%.d)

## build rules
TARGET = $(SIM_ROOT)/lib/pin_sim.so

all: $(TARGET)

$(CARBON_LIB):
	make -C $(SIM_ROOT)/common

$(CONTRIB_LIBS):
	make -C $(SIM_ROOT)/contrib

$(TARGET): $(CARBON_LIB) $(CONTRIB_LIBS) $(OBJECTS)
	@echo $(TARGET)
	$(CXX) $(DBG_FLAGS) $(OPT_FLAGS) $(TOOL_LDFLAGS) $(LD_FLAGS) -o $@ $(OBJECTS) $(TOOL_LPATHS) $(TOOL_LIBS) $(LD_LIBS)

ifeq ($(CLEAN),)
-include $(OBJECTS:%.o=%.d)
endif

ifneq ($(CLEAN),)
clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDENCIES)
endif
