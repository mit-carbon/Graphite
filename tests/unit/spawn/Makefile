
BUILD_32 = 1

#SIM_ROOT := $(PWD)/../../../
SIM_ROOT := $(PWD)

OS_SERVICES_ROOT = $(SIM_ROOT)/os-services-25032-gcc.4.0.0-linux-ia32_intel64
BOOST_ROOT = /afs/csail/group/carbon/tools/boost_1_38_0

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

.SUFFIXES:  .o .c .h .cc

all: spawn

# Add support for os services
LD_LIBS += -los-services
ifeq ($(BUILD_32),1)
  CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-ia32
  LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/ia32 -m32
else
  CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-intel64
  LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/intel64
endif

# Boost support
CXXFLAGS += -I$(BOOST_ROOT)
BOOST_SUFFIX = gcc41-mt-1_38
LD_FLAGS += -L$(BOOST_ROOT)/stage/lib
LD_LIBS += -lboost_filesystem-$(BOOST_SUFFIX) -lboost_system-$(BOOST_SUFFIX) -pthread  -lcarbon_sim


# Mpi compiler
MPI_DIR = /afs/csail/group/carbon/tools/mpich2-1.0.8/install
#export LD_LIBRARY_PATH := $(MPI_DIR)/lib  ####:$(LD_LIBRARY_PATH)
CC = $(MPI_DIR)/bin/mpicc
CXX = $(MPI_DIR)/bin/mpicxx

spawn: $(OBJECTS) $(SUPPORT_OBJECTS)
	$(CXX) $^ -o $@ $(LD_FLAGS) $(LD_LIBS) 

show_pinflags:
	@echo $(PIN_LDFLAGS)

clean:
	-rm spawn ./*.o
