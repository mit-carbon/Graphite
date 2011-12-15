TARGET = raytrace

OBJECTS = bbox.o cr.o env.o fbuf.o geo.o huprn.o husetup.o hutv.o isect.o \
	main.o matrix.o memory.o poly.o raystack.o shade.o sph.o trace.o \
	tri.o workpool.o globals.o
APP_FLAGS ?= -p64 -m64 $(CURDIR)/inputs/car.env

#MACROS := ../splash_support/c.m4.null.CARBON
MACROS := ../splash_support/c.m4.null.POSIX

include ../../Makefile.tests

include ../splash_support/Makefile.config

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

