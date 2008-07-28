include ../makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS=-c -I$(PIN_HOME)/InstLib -I../../pin/src -I../user -I../network -I../phys_trans -I../misc -I../ -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = cache.cc ocache.cc perfmdl.cc core.cc memory_manager.cc address_home_lookup.cc cache_directory.cc cache_directory_entry.cc dram_directory.cc dram_directory_entry.cc
OBJECTS = $(SOURCES:%.cc=%.o)


all: $(OBJECTS)


## build rules

%.o : %.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo
squeaky: clean
	-rm -f *~
