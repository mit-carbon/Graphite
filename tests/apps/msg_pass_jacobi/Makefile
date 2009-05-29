TARGET=jacobi
SOURCES = jacobi.cc
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

SIZE ?= 4
APP_FLAGS = -m $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
