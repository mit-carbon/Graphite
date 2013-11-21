ifneq ($(USE_GRAPHITE),)
	TARGET = libmcpat.a
else
	TARGET = mcpat
endif

SHELL = /bin/bash
.PHONY: all clean
.SUFFIXES: .cc .o

LIBS = -lm -pthread

NTHREADS ?= 4

ifeq ($(TAG),dbg)
	DBG = -ggdb
	OPT = -O0 -DNTHREADS=1 -Icacti
else
	OPT = -O3 -msse2 -mfpmath=sse -DNTHREADS=$(NTHREADS) -Icacti
endif

ifneq ($(USE_GRAPHITE),)
	OPT += -DUSE_GRAPHITE
endif

CXXFLAGS = -fPIC -Wno-unknown-pragmas $(DBG) $(OPT)
CXX = g++

VPATH = cacti

SRCS  = \
  Ucache.cc \
  XML_Parse.cc \
  arbiter.cc \
  area.cc \
  array.cc \
  bank.cc \
  basic_circuit.cc \
  basic_components.cc \
  cacti_interface.cc \
  component.cc \
  core.cc \
  crossbar.cc \
  decoder.cc \
  htree2.cc \
  interconnect.cc \
  io.cc \
  iocontrollers.cc \
  logic.cc \
  mat.cc \
  memoryctrl.cc \
  noc.cc \
  nuca.cc \
  parameter.cc \
  processor.cc \
  router.cc \
  sharedcache.cc \
  subarray.cc \
  technology.cc \
  uca.cc \
  wire.cc \
  xmlParser.cc \
  powergating.cc \
  core_wrapper.cc \
  cache_wrapper.cc

ifeq ($(USE_GRAPHITE),)
	SRCS += main.cc
endif

OBJS = $(patsubst %.cc,obj_$(TAG)/%.o,$(SRCS))

all: obj_$(TAG)/$(TARGET)
	@echo $(OBJS)
	cp -f obj_$(TAG)/$(TARGET) $(TARGET)

ifneq ($(USE_GRAPHITE),)
obj_$(TAG)/$(TARGET): $(OBJS)
	ar rcs $@ $^
else
obj_$(TAG)/$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LIBS)
endif

obj_$(TAG)/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj_$(TAG) libmcpat.a mcpat
