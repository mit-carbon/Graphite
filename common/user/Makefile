include ../makefile.gnu.config

DBG=-g
OPT=-O2
CXXFLAGS=-c -I../core -I../transport -I../misc -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

C_SOURCES = capi.c sync_api.c
CPP_SOURCES = 

OBJECTS = $(CPP_SOURCES:%.cc=%.o) $(C_SOURCES:%.c=%.o)

include ../makefile.common

