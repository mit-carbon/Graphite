include ../makefile.gnu.config
include ../makefile.debug

DBG=-g
OPT=-O2
CFLAGS+=-c -I../../pin/src -I../user -I../network -I../phys_trans -I../misc -I../ -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS= 

#<<<<<<< HEAD:common/core/Makefile
SOURCES = cache.cc ocache.cc perfmdl.cc core.cc memory_manager.cc address_home_lookup.cc dram_directory.cc dram_directory_entry.cc debug.cc syscall_model.cc
#=======
#SOURCES = cache.cc ocache.cc perfmdl.cc core.cc syscall_model.cc
#>>>>>>> master:common/core/Makefile
OBJECTS = $(SOURCES:%.cc=%.o)


all: $(OBJECTS)


## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo
squeaky: clean
	-rm -f *~
