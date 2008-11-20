include ../makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS=-c -I../phys_trans -I../misc -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

C_SOURCES = capi.c
CPP_SOURCES = mcp_api.cc sync_api.cc user_api.cc
OBJECTS = $(CPP_SOURCES:%.cc=%.o) $(C_SOURCES:%.c=%.o)

all: $(OBJECTS)


## build rules

%.o : %.cc
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

clean:
	-rm -f *.o *.d *.rpo
squeaky: clean
	-rm -f *~


