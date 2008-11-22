TARGET = OCEAN
OBJS = jacobcalc.o jacobcalc2.o laplacalc.o linkup.o main.o multi.o slave1.o slave2.o subblock.o

MACROS := ../splash_support/c.m4.null.CARBON
include ../splash_support/Makefile.config
include ../../makefile.gnu.config
TARGET = OCEAN

CFLAGS = -I../../user -I../../phys_trans -c -g -Wall

decs.h: decs.H
jacobcalc.c: decs.h
linkup.c: decs.h
slave1.c: decs.h
jacobcalc2.c: decs.h
main.c: decs.h
slave2.c: decs.h
laplacalc.c: decs.h
multi.c: decs.h
subblock.c: decs.h

