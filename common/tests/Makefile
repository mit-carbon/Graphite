include ../makefile.gnu.config

all: pthreadsmatmult


pthreadsmatmult:
	$(MAKE) -C pthreads_matmult

clean:
	$(MAKE) -C pthreads_matmult clean
	-rm -f *.o $(TOOLS) *.d *.rpo

squeaky: clean
	$(MAKE) -C pthreads_matmult squeaky
	-rm -f *~
