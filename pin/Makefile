include ../common/makefile.gnu.config
include ../common/makefile.debug

all:
	-mkdir -f bin
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	-rm -rf bin/*
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C src squeaky
	-rm -f *~

