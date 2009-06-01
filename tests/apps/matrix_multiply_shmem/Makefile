TARGET=mmultiply
SOURCES = mmultiply.c
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

SIZE ?= 4
APP_FLAGS ?= 4 4 $(SIZE) $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))
ENABLE_SM = true

include ../../Makefile.tests
