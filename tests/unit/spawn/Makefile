
BUILD_32 = 1

#SIM_ROOT := $(PWD)/../../../
SIM_ROOT := $(PWD)

LD_LIBS = 
LD_FLAGS = -L$(SIM_ROOT)/lib 

SOURCES = spawn.cc thread_spawner.cc
OBJECTS = $(SOURCES:%.cc=%.o) $(SRCOBJS)

SUPPORT_SOURCES = $(SIM_ROOT)/pin/src/pin_thread.cc

SUPPORT_OBJECTS = $(SUPPORT_SOURCES:%.cc=%.o)

CXXFLAGS = -I$(SIM_ROOT)/common/system     \
           -I$(SIM_ROOT)/common/user       \
           -I$(SIM_ROOT)/common/transport  \
           -I$(SIM_ROOT)/common/network    \
           -I$(SIM_ROOT)/common/misc       \
           -I$(SIM_ROOT)/common/config     \
           -I$(SIM_ROOT)/common/core       \
           -I$(SIM_ROOT)/common/../pin/src \
           -c -ggdb -Wall -m32

CXXFLAGS += -DSIM_STANDALONE

CFLAGS = $(CXXFLAGS)

include ../../../common/makefile.common

.SUFFIXES:  .o .c .h .cc

all: spawn

spawn: $(OBJECTS) $(SUPPORT_OBJECTS)
	$(CXX) $^ -o $@ $(LD_FLAGS) $(LD_LIBS) 

show_pinflags:
	@echo $(PIN_LDFLAGS)

clean:
	-rm spawn ./*.o
