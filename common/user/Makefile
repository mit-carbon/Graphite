include ../makefile.gnu.config

SIM_ROOT ?= $(CURDIR)/../../

DBG=-g
OPT=-O2
CXXFLAGS=-c -I../ -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
CFLAGS=$(CXXFLAGS)
LDFLAGS=

C_SOURCES = capi.c sync_api.c thread_support.c
CPP_SOURCES = 

OBJECTS = $(CPP_SOURCES:%.cc=%.o) $(C_SOURCES:%.c=%.o)

include ../makefile.common

