TARGET=ring
SOURCES = ring.c
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

SIZE ?= 2
APP_FLAGS = -m $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
