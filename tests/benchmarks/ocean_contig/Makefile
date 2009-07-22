TARGET = ocean_contig
OBJECTS = jacobcalc.o jacobcalc2.o laplacalc.o linkup.o main.o multi.o slave1.o slave2.o subblock.o
APP_FLAGS = -p8

MACROS := ../splash_support/c.m4.null.CARBON
#MACROS := ../splash_support/c.m4.null.POSIX

include ../splash_support/Makefile.config

include ../../Makefile.tests

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

