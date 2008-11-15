TARGET = FMM
OBJS = box.o construct_grid.o cost_zones.o defs.o fmm.o interactions.o memory.o particle.o partition_grid.o


MACROS := ../splash_support/c.m4.null.CARBON
include ../splash_support/Makefile.config
include ../../makefile.gnu.config
TARGET = FMM

CFLAGS = -I../../user -I../../phys_trans -c -g -Wall
#ifeq (${FORCE32},1)
#  CFLAGS = -I../../user -I../../phys_trans -c -g -Wall -m32
#  LDFLAGS = -m32 -m elf_i386
##else
#  CFLAGS = -I../../user -I../../phys_trans -c -g -Wall
#  LDFLAGS = 
#endif

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
