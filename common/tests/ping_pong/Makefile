#<<<<<<< HEAD:common/tests/ping_pong/Makefile
TARGET=ping_pong

C_SOURCES = 
CPP_SOURCES = ping_pong.cc 
OBJECTS = $(CPP_SOURCES:%.cc=%.o) $(C_SOURCES:%.c=%.o) 
#=======
#include ../../makefile.gnu.config
##include ../../makefile.debug
#
#FORCE32 ?= 1
#CXX = /afs/csail/group/carbon/tools/openmpi/bin/mpicxx
#
#ifeq (${FORCE32},1)
#  CFLAGS = -I../../phys_trans -I../../network -I../../misc -I../../core -I../../../pin/src -I../../user -c -g -Wall -m32
#  LDFLAGS = -m32 -m elf_i386
#else
#  CFLAGS = -I../../phys_trans -I../../network -I../../misc -I../../core -I../../../pin/src -I../../user -c -g -Wall
#  LDFLAGS = 
#endif
#
#OLD_SOURCES = ../../user/mcp_api.cc ../../user/capi.cc ping_pong.cc
#OLD_OBJECTS = $(OLD_SOURCES:%.cc=%.o)
#>>>>>>> shared_mem:common/tests/ping_pong/Makefile

include ../Makefile.common

#<<<<<<< HEAD:common/tests/ping_pong/Makefile
#=======
#all: ping_pong 
#
#%.o : %.cc
#	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<
#
#ping_pong: $(OLD_OBJECTS)
#	$(CXX) -pthread $(LDFLAGS) -o $@ $(OLD_OBJECTS)
#
### Cleaning
#
#clean:
#	-rm -f *.o ping_pong sim.out
#
#squeaky: clean
#	-rm -f *~
#>>>>>>> shared_mem:common/tests/ping_pong/Makefile
