TARGET = BARNES
OBJS = code.o code_io.o load.o grav.o getparam.o util.o

include Makefile.config

stdinc.h: code.h defs.h util.h vectmath.h load.h code_io.h grav.h getparam.h stdinc.H 
code.o: code.C stdinc.h
code_io.o: code_io.C stdinc.h
getparam.o: getparam.C stdinc.h
grav.o: grav.C stdinc.h
load.o: load.C stdinc.h
util.o: util.C stdinc.h

