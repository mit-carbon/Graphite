TARGET=capi_worker
SOURCES = capi_worker.c
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )
include ../../Makefile.tests

