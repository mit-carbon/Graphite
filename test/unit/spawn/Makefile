
SIM_ROOT := $(PWD)/../../../

LD_LIBS = 
LD_FLAGS = -L$(SIM_ROOT)/lib 

SOURCES = spawn.cc
OBJECTS = $(SOURCES:%.cc=%.o) $(SRCOBJS)

SUPPORT_SOURCES = $(SIM_ROOT)/pin/src/pin_thread.cc \
                  $(SIM_ROOT)/lib/pthread_lock.cc


SUPPORT_OBJECTS = $(SUPPORT_SOURCES:%.cc=%.o)

CXXFLAGS = -I$(SIM_ROOT)/common/system     \
           -I$(SIM_ROOT)/common/user       \
           -I$(SIM_ROOT)/common/transport  \
           -I$(SIM_ROOT)/common/network    \
           -I$(SIM_ROOT)/common/misc       \
           -I$(SIM_ROOT)/common/config     \
           -I$(SIM_ROOT)/common/core       \
           -I$(SIM_ROOT)/common/../pin/src \
           -I/afs/csail/group/carbon/tools/boost_1_38_0/  \
           -c -ggdb -Wall -m32

.SUFFIXES:  .o .c .h .cc

all: spawn

#fixme -- this shouldn't be here
#include $(SIM_ROOT)/common/makefile.gnu.config

OS_SERVICES_ROOT = ../../../os-services-25032-gcc.4.0.0-linux-ia32_intel64

CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-ia32
LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/ia32 -m32

#CXXFLAGS += -I$(OS_SERVICES_ROOT)/include-intel64
#LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/intel64

BOOST_ROOT = /afs/csail/group/carbon/tools/boost_1_38_0/stage/lib
BOOST_SUFFIX = gcc41-mt-1_38
LD_FLAGS += -L$(BOOST_ROOT)
LD_LIBS += -lboost_filesystem-$(BOOST_SUFFIX) -lboost_system-$(BOOST_SUFFIX) -pthread  -lcarbon_sim

#EXTRA_PINFLAGS=-g -m32  -Wl,--version-script=/afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux/source/include/pintool.ver -L/afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux/Lib/ -L/afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux/ExtLib/ -L/afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux/extras/xed2-ia32/lib -L/afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux/ia32/lib -L/afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux/ia32/lib-ext
#LD_FLAGS += $(EXTRA_PINFLAGS)

#CXXFLAGS+= -m32 $(PIN_CXXFLAGS)
#CFLAGS=$(CXXFLAGS) 

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
