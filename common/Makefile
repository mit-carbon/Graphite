include ./makefile.gnu.config

all: userobjs coreobjs networkobjs phystransobjs miscobjs testobjs


userobjs:
	$(MAKE) -C user

coreobjs:
	$(MAKE) -C core

networkobjs:
	$(MAKE) -C network

phystransobjs:
	$(MAKE) -C phys_trans

miscobjs:
	$(MAKE) -C misc

testobjs:
	$(MAKE) -C tests

clean:
	$(MAKE) -C user clean
	$(MAKE) -C core clean
	$(MAKE) -C network clean
	$(MAKE) -C phys_trans clean
	$(MAKE) -C misc clean
	$(MAKE) -C tests clean
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C user squeaky
	$(MAKE) -C core squeaky
	$(MAKE) -C network squeaky
	$(MAKE) -C phys_trans squeaky
	$(MAKE) -C misc squeaky
	$(MAKE) -C tests squeaky
	-rm -f *~
