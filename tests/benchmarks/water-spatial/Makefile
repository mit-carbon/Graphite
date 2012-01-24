TARGET = water-spatial
OBJECTS = bndry.o cnstnt.o cshift.o initia.o interf.o intraf.o kineti.o mdmain.o poteng.o predcor.o syscons.o water.o
APP_FLAGS ?= \< $(CURDIR)/input

#MACROS := ../splash_support/c.m4.null.CARBON
MACROS := ../splash_support/c.m4.null.POSIX

include ../../Makefile.tests

include ../splash_support/Makefile.config

bndry.o: bndry.C split.h mdvar.h parameters.h mddata.h global.h
cnstnt.o: cnstnt.C water.h wwpot.h cnst.h frcnst.h fileio.h parameters.h global.h
cshift.o: cshift.C water.h global.h
initia.o: initia.C split.h mdvar.h water.h cnst.h fileio.h parameters.h mddata.h global.h
interf.o: interf.C mdvar.h water.h wwpot.h cnst.h global.h split.h parameters.h mddata.h
intraf.o: intraf.C frcnst.h mdvar.h water.h wwpot.h parameters.h mddata.h global.h split.h
kineti.o: kineti.C global.h split.h mdvar.h parameters.h mddata.h
mdmain.o: mdmain.C  parameters.h mdvar.h water.h wwpot.h cnst.h mddata.h fileio.h frcnst.h randno.h global.h split.h
poteng.o: poteng.C global.h split.h mdvar.h frcnst.h water.h wwpot.h parameters.h mddata.h
predcor.o: predcor.C split.h mdvar.h parameters.h mddata.h global.h
syscons.o: syscons.C parameters.h mdvar.h water.h wwpot.h cnst.h mddata.h global.h
water.o: water.C parameters.h mdvar.h water.h wwpot.h cnst.h mddata.h fileio.h frcnst.h randno.h global.h split.h
