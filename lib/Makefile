
SIM_ROOT ?= $(PWD)/

CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/makefile.common
endif

# Grab all c/c++ files from subdirs
C_SOURCES = 
CPP_SOURCES =                        \
          ../common/core/*.cc        \
          ../common/network/*.cc     \
          ../common/transport/*.cc   \
          ../common/misc/*.cc        \
          ../common/config/*.cpp     \
          ../common/system/*.cc

# Grab the actual filenames and convert them into .o files for dependencies
SOURCES = $(shell ls $(CPP_SOURCES) $(C_SOURCES))

# Convert the .c and .cc's in to .o's
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

CXXFLAGS+=-c -I$(SIM_ROOT)/common/user      \
          -I$(SIM_ROOT)/common/system       \
          -I$(SIM_ROOT)/common/core         \
          -I$(SIM_ROOT)/common/network      \
          -I$(SIM_ROOT)/common/transport    \
          -I$(SIM_ROOT)/common/misc         \
          -I$(SIM_ROOT)/common/config       \
          -I /afs/csail/group/carbon/tools/mpich2-1.0.8/install/include/ \
          -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD -m32 -ggdb

# targets
all: libcarbon_sim.a

libcarbon_sim.a: $(OBJECTS)
	ar rcs $@ $^

ifneq ($(CLEAN),)
clean:
	-rm -f libcarbon_sim.a $(OBJECTS)
endif


