SIM_ROOT ?= $(CURDIR)/../../
TEST_BENCH_DIR = $(SIM_ROOT)/tests/benchmarks/

TEST_BENCH_LIST = barnes_bench_test ocean_contiguous_bench_test ocean_non_contiguous_bench_test water-nsquared_bench_test water-spatial_bench_test cholesky_bench_test fft_bench_test lu_contiguous_bench_test lu_non_contiguous_bench_test radix_bench_test volrend_bench_test raytrace_bench_test radiosity_bench_test
TEST_DIST_BENCH_LIST = barnes_bench_test_dist ocean_contiguous_bench_test_dist ocean_non_contiguous_bench_test_dist water-nsquared_bench_test_dist water-spatial_bench_test_dist cholesky_bench_test_dist fft_bench_test_dist lu_contiguous_bench_test_dist lu_non_contiguous_bench_test_dist radix_bench_test_dist volrend_bench_test_dist raytrace_bench_test_dist

# Radiosity is not added to the TEST_DIST_BENCH_LIST since it takes 3 days to complete

regress_bench: $(TEST_BENCH_LIST) $(TEST_DIST_BENCH_LIST)

ifeq ($(MAKECMDGOALS),clean)
clean:
	for t in $(patsubst %_bench_test,%,$(TEST_BENCH_LIST)) ; do make -C $(TEST_BENCH_DIR)/$$t clean ; done
endif

%_bench_test_dist:
	date
	$(MAKE) -C $(TEST_BENCH_DIR)/$(patsubst %_bench_test_dist,%,$@) PROCS=2; if [ $$? -ne 0 ] ; then echo "TEST: $@ FAILED" ; else echo "TEST: $@ PASSED" ; true ; fi
%_bench_test:
	date
	$(MAKE) -C $(TEST_BENCH_DIR)/$(patsubst %_bench_test,%,$@); if [ $$? -ne 0 ] ; then echo "TEST: $@ FAILED" ; else echo "TEST: $@ PASSED" ; true ; fi
