include ../makefile.gnu.config

DBG=-g
OPT=-O2
CXXFLAGS+=-c -I../../pin/src -I../user -I../system -I../core -I../transport -I../misc -I../ -I./ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = network.cc network_model.cc network_model_analytical.cc
OBJECTS = $(SOURCES:%.cc=%.o)

include ../Makefile.common

