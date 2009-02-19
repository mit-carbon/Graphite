.PHONY: use-mpi use-sm pick-version
include ../makefile.gnu.config

## Use this option to select the version of the PT layer you want to use ##
## Choices are: mpi, sm
PT_VERSION ?= sm

DBG=-g
OPT=-O2
CXXFLAGS=-c -I../core -I../misc -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = transport.cc mpitransport.cc
OBJECTS = $(SOURCES:%.cc=%.o)

include ../makefile.common

