include ../makefile.gnu.config

all: pthreadsmatmult pingpong


pthreadsmatmult:
	$(MAKE) -C pthreads_matmult

pingpong:
	$(MAKE) -C ping_pong

clean:
	$(MAKE) -C pthreads_matmult clean
	-rm -f *.o $(TOOLS) *.d *.rpo

squeaky: clean
	$(MAKE) -C pthreads_matmult squeaky
	-rm -f *~
