TARGET = fmm
OBJECTS = box.o construct_grid.o cost_zones.o defs.o fmm.o interactions.o memory.o particle.o partition_grid.o
APP_FLAGS ?= \< $(CURDIR)/inputs/input.16384

#MACROS := ../splash_support/c.m4.null.CARBON
MACROS := ../splash_support/c.m4.null.POSIX

include ../../Makefile.tests

include ../splash_support/Makefile.config

all:
	rm -f $(SIM_ROOT)/times
	$(RUN)

defs.c: defs.h
box.c: box.h
construct_grid.c: construct_grid.h
cost_zones.c: cost_zones.h
interactions.c: interactions.h
memory.c: memory.h
particle.c: particle.h
partition_grid.c: partition_grid.h

fmm.o: fmm.C defs.h memory.h particle.h box.h partition_grid.h cost_zones.h construct_grid.h interactions.h
interactions.o: interactions.C defs.h memory.h particle.h box.h partition_grid.h interactions.h
shell.o: shell.C defs.h memory.h particle.h box.h partition_grid.h interactions.h
construct_grid.o: construct_grid.C defs.h memory.h particle.h box.h partition_grid.h construct_grid.h
cost_zones.o: cost_zones.C defs.h memory.h box.h partition_grid.h cost_zones.h
#orb.o: orb.C defs.h memory.h box.h partition_grid.h orb.h
partition_grid.o: partition_grid.C defs.h memory.h box.h partition_grid.h
box.o: box.C defs.h memory.h particle.h box.h
particle.o: particle.C defs.h memory.h particle.h
memory.o: memory.C defs.h memory.h
defs.o: defs.C defs.h memory.h
memory.h: defs.h particle.h box.h
