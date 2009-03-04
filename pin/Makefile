include ../common/makefile.gnu.config

bin:
	mkdir bin

all: bin
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	-rm -rf bin/*
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C src squeaky
	-rm -f *~

