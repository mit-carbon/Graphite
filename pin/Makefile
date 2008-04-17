include ../common/makefile.gnu.config

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C src squeaky
	-rm -f *~

