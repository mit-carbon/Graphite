FORCE32 ?= 1

# Change this if you move to a new system or upgrade Pin
PINDIR = /afs/csail/group/carbon/tools/pin/current

# Number of cores to simulate
NC = 4

CC = /usr/bin/gcc
ifeq (${FORCE32},1)
  CFLAGS = -I../../user -c -g -Wall -m32
  LDFLAGS = -m32 -m elf_i386
  TARGET = ia32
else
  CFLAGS = -I../../user -c -g -Wall
  LDFLAGS = 
  TMP=$(shell uname -m)
  ifeq (${TMP},x86_64)
    TARGET = ia32e
  else
    TARGET = ia32
  endif
endif
SOURCES = ../../user/capi.c ring_c.c
OBJECTS = $(SOURCES:%.c=%.o)

MPI_DIR = /afs/csail/group/carbon/tools/openmpi/$(TARGET)

## Build rules

all: ring_c

%.o : %.c
	$(CC) $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

ring_c: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

# Rule to run app using MPI under Pin.  Tell MPI and the app about the total
# number of cores but tell the pintool that we only have 1 thread (we want
# 1 thread per process with NC processes).
run:
	$(MPI_DIR)/bin/mpirun -np $(NC) $(PINDIR)/pin -t ../../../pin/bin/pin_sim -n 1 -np $(NC) -tc $(NC) -- ./ring_c -m $(NC)

## Cleaning

clean:
	-rm -f *.o ring_c sim.out

squeaky: clean
	-rm -f *~ pin.log pintool.log
