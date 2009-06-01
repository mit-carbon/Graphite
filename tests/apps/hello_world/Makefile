TARGET = hello_world
SOURCES = hello_world.c
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )
include ../../Makefile.tests
