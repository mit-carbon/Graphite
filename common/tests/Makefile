
all: 

include ../makefile.gnu.config
shared_mem_tests:
	$(MAKE) -C shared_mem_test

basic:
	$(MAKE) -C pthreads_basic

pingpong:
	$(MAKE) -C ping_pong

jacobi:
	$(MAKE) -C shared_mem_jacobi

clean:
	$(MAKE) -C shared_mem_test clean
	$(MAKE) -C ping_pong clean
	$(MAKE) -C capi_worker clean
	$(MAKE) -C pthreads_basic clean
	$(MAKE) -C shared_mem_jacobi clean
	rm -f *.o $(TOOLS) *.d *.rpo

clean_benchmarks:
	$(MAKE) -C fmm clean
	$(MAKE) -C barnes clean
	$(MAKE) -C radiosity clean
	$(MAKE) -C raytrace clean
	$(MAKE) -C volrend clean
	$(MAKE) -C ocean_contig clean
	$(MAKE) -C water-nsquared clean
	$(MAKE) -C water-spatial clean

build_benchmarks:
	$(MAKE) -C fmm
	$(MAKE) -C barnes
	$(MAKE) -C radiosity
	$(MAKE) -C ocean_contig
	$(MAKE) -C raytrace
	$(MAKE) -C volrend
	$(MAKE) -C water-nsquared
	$(MAKE) -C water-spatial
