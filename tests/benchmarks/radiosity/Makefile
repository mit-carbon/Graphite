TARGET = radiosity
OBJECTS = display.o elemman.o modelman.o patchman.o rad_main.o rad_tools.o room_model.o smallobj.o taskman.o visible.o
APP_FLAGS ?= -p 64 -batch -room

#MACROS := ../splash_support/c.m4.null.CARBON
MACROS := ../splash_support/c.m4.null.POSIX

include ../../Makefile.tests

include ../splash_support/Makefile.config

$(TARGET): glibps/glibps.a glibdumb/glib.a

CFLAGS += -I$(CURDIR)/glibps -I$(CURDIR)/glibdumb
LD_FLAGS += $(CURDIR)/glibps/glibps.a $(CURDIR)/glibdumb/glib.a

HHS = model.H parallel.H patch.H radiosity.H task.H
HS = model.h parallel.h patch.h radiosity.h task.h

radiosity.h: patch.h model.h parallel.h task.h
rad_main.c: rad_main.C $(HS)
patchman.c: patchman.C $(HS)
elemman.c: elemman.C $(HS)
taskman.c: taskman.C $(HS)
modelman.c: modelman.C $(HS)
smallobj.c: smallobj.C $(HS)
display.c: display.C $(HS)
visible.c: visible.C $(HS)
rad_tools.c: rad_tools.C $(HS)
room_model.c: room_model.C $(HS)

glibps/glibps.a:
	make -C glibps

glibdumb/glib.a:
	make -C glibdumb

clean:
	$(RM) *.d *.o *.c *.h
	make -C glibps clean
	make -C glibdumb clean
