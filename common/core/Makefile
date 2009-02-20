include ../makefile.gnu.config
include ../makefile.debug

DBG=-g
OPT=-O2
CXXFLAGS+=-c -I../../pin/src -I../user -I../system -I../network -I../phys_trans -I../misc -I../ -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS= 
SOURCES = cache.cc ocache.cc perfmdl.cc core.cc memory_manager.cc address_home_lookup.cc dram_directory.cc dram_directory_entry.cc syscall_model.cc
OBJECTS = $(SOURCES:%.cc=%.o)

include ../makefile.common

