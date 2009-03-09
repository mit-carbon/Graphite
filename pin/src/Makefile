
# this gives us default build rules and dependency handling
SIM_ROOT ?= $(CURDIR)/../../

LD_LIBS += -pthread -lcarbon_sim

CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

DBG=-g
OPT=-O2
BUILD_32 = 1

# Use these files for auto targets
.SUFFIXES:  .o .c .h .cc

# Add other CXX Flags
CXXFLAGS += -c -fomit-frame-pointer  \
            -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

# Use the pin flags for building
include ../../common/makefile.gnu.config
CXXFLAGS += $(PIN_CXXFLAGS)
CFLAGS += $(CXXFLAGS)


SOURCES = pin_sim.cc analysis.cc routine_replace.cc run_models.cc  \
          user_space_wrappers.cc pin_thread.cc handle_args.o

OBJECTS = $(SOURCES:%.cc=%.o)

## build rules
BUILD_TARGET = ../bin/pin_sim.so

../bin:
	-mkdir ../bin

all: $(BUILD_TARGET) ../bin

$(BUILD_TARGET): $(PIN_LIBNAMES)
$(BUILD_TARGET): $(OBJECTS)
	$(CXX) $(PIN_LDFLAGS) $(LD_FLAGS) -o $@ $^ $(PIN_LIBS) $(LD_LIBS) $(DBG)

ifneq ($(CLEAN),)
clean:
	-rm -f $(BUILD_TARGET) $(OBJECTS)
endif
