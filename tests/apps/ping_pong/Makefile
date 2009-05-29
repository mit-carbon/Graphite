TARGET=ping_pong
SOURCES = ping_pong.cc 
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

ENABLE_SM = true

include ../../Makefile.tests
