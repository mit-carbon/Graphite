include ../makefile.gnu.config

all: pthreadsmatmult pingpong basic shared_mem_tests

shared_mem_tests:
	$(MAKE) -C shared_mem_test

basic:
	$(MAKE) -C pthreads_basic

pthreadsmatmult:
	$(MAKE) -C pthreads_matmult

pingpong:
	$(MAKE) -C ping_pong

jacobi:
	$(MAKE) -C shared_mem_jacobi

clean:
	$(MAKE) -C pthreads_matmult clean
	$(MAKE) -C shared_mem_test clean
	$(MAKE) -C ping_pong clean
	$(MAKE) -C pthreads_basic clean
	$(MAKE) -C shared_mem_jacobi clean
	-rm -f *.o $(TOOLS) *.d *.rpo

squeaky: clean
	$(MAKE) -C pthreads_matmult squeaky
	-rm -f *~
