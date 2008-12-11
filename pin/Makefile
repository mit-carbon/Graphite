include ../common/makefile.gnu.config
include ../common/makefile.debug

all:
	-rm -rf bin
	-mkdir bin
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	-rm -rf bin
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C src squeaky
	-rm -f *~

