include $(CURDIR)/makefile.gnu.config

sim: userobjs coreobjs networkobjs transportobjs miscobjs  sysobjs
all: sim

userobjs:
	$(MAKE) -C user

coreobjs:
	$(MAKE) -C core

networkobjs:
	$(MAKE) -C network

transportobjs:
	$(MAKE) -C transport

sysobjs:
	$(MAKE) -C system

miscobjs:
	$(MAKE) -C misc

clean:
	$(MAKE) -C user clean
	$(MAKE) -C core clean
	$(MAKE) -C network clean
	$(MAKE) -C transport clean
	$(MAKE) -C misc clean
	$(MAKE) -C system clean
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C user squeaky
	$(MAKE) -C core squeaky
	$(MAKE) -C network squeaky
	$(MAKE) -C transport squeaky
	$(MAKE) -C misc squeaky
	$(MAKE) -C system squeaky
	-rm -f *~

