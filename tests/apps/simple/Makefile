TARGET=simple_test
SOURCES = simple_test.c
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )
include ../../Makefile.tests

