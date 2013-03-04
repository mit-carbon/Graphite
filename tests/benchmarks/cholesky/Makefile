TARGET = cholesky
OBJECTS = amal.o assign.o bfac.o bksolve.o block2.o fo.o malloc.o \
       mf.o numLL.o parts.o seg.o solve.o tree.o util.o
APP_FLAGS ?= -p64 $(CURDIR)/inputs/tk29.O

#MACROS := ../splash_support/c.m4.null.CARBON
MACROS := ../splash_support/c.m4.null.POSIX

include ../../Makefile.tests

include ../splash_support/Makefile.config

matrix.h: matrix.H
amal.c: matrix.h
block2.c: matrix.h
numLL.c: matrix.h
tree.c: matrix.h
assign.c: matrix.h
fo.c: matrix.h
parts.c: matrix.h
util.c: matrix.h
bfac.c: matrix.h
malloc.c: matrix.h
seg.c: matrix.h
bksolve.c: matrix.h
mf.c: matrix.h
solve.c: matrix.h

