include ../makefile.gnu.config

#all: pthreadsmatmult pingpong basic shared_mem_tests
all: pingpong basic shared_mem_tests

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
	#$(MAKE) -C pthreads_matmult clean
	$(MAKE) -C shared_mem_test clean
	$(MAKE) -C ping_pong clean
	$(MAKE) -C pthreads_basic clean
	$(MAKE) -C shared_mem_jacobi clean
	$(MAKE) -C fmm clean
	$(MAKE) -C barnes clean
	$(MAKE) -C ocean_contig clean
	$(MAKE) -C water-nsquared clean
	$(MAKE) -C water-spatial clean
	-rm -f *.o $(TOOLS) *.d *.rpo

squeaky: clean
	$(MAKE) -C pthreads_matmult squeaky
	-rm -f *~
