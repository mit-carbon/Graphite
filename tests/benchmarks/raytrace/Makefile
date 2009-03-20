SIM_ROOT ?= $(CURDIR)/../../../
TARGET = RAYTRACE

OBJS = bbox.o cr.o env.o fbuf.o geo.o huprn.o husetup.o hutv.o isect.o \
	main.o matrix.o memory.o poly.o raystack.o shade.o sph.o trace.o \
	tri.o workpool.o globals.o

MACROS := ../splash_support/c.m4.null.CARBON
include ../splash_support/Makefile.config
TARGET = RAYTRACE

CFLAGS = -I$(SIM_ROOT)/common/user -I$(SIM_ROOT)/common/misc -c -g -Wall

bbox.c: bbox.C rt.h
cr.c:	cr.C rt.h
globals.c: globals.C rt.h
env.c:	env.C rt.h
fbuf.c: fbuf.C rt.h
geo.c:	geo.C rt.h
huprn.c: huprn.C rt.h
husetup.c: husetup.C rt.h
hutv.c: hutv.C rt.h
isect.c: isect.C rt.h
main.c: main.C rt.h
matrix.c: matrix.C rt.h
memory.c: memory.C rt.h
poly.c: poly.C rt.h
raystack.c: raystack.C rt.h
shade.c: shade.C rt.h
sph.c:	sph.C rt.h
trace.c: trace.C rt.h
tri.c:	tri.C rt.h
workpool.c: workpool.C rt.h

