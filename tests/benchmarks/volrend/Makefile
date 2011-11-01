TARGET = volrend
OBJECTS = adaptive.o file.o main.o map.o normal.o octree.o opacity.o option.o raytrace.o render.o view.o
APP_FLAGS ?= 64 $(CURDIR)/inputs/head

#MACROS := ../splash_support/c.m4.null.CARBON
MACROS := ../splash_support/c.m4.null.POSIX

include ../../Makefile.tests

include ../splash_support/Makefile.config

$(TARGET): libtiff/libtiff.a

CFLAGS += -I$(CURDIR)/libtiff
LD_FLAGS += -L$(CURDIR)/libtiff -ltiff

main.c:	main.C incl.h anl.h
file.c:	file.C incl.h
option.c: option.C incl.h
map.c: map.C incl.h
normal.c: normal.C incl.h anl.h address.h
opacity.c: opacity.C incl.h anl.h
octree.c: octree.C incl.h anl.h
view.c:	view.C incl.h
render.c: render.C incl.h
adaptive.c: adaptive.C incl.h anl.h
raytrace.c: raytrace.C incl.h address.h

libtiff/libtiff.a:
	make -C libtiff

incl.h:	user_options.h const.h my_types.h global.h macros.h address.h
tiffio.h:

clean:
	$(RM) -f *.d *.o *.c *.h
	make -C libtiff clean
