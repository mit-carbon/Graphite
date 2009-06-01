TARGET=jacobi
SOURCES = jacobi.c
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

SIZE ?= 4
APP_FLAGS ?= $(SIZE) 64
CORES = $(shell echo $$(( $(SIZE) + 1 )))
ENABLE_SM = true

include ../../Makefile.tests
