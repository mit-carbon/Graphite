TARGET = open_mp
SOURCES = $(TARGET).cc
CORES ?= 2
SIM_ROOT ?= $(CURDIR)/../../..

include ../../Makefile.tests

CXXFLAGS += -fopenmp
LD_FLAGS += -fopenmp

$(TARGET): open_mp.o
	$(CXX) -o $@ $(LD_FLAGS) $< $(LD_LIBS)

open_mp.o: open_mp.cc
	$(CXX) -c -o $@ $(CXXFLAGS) $<
