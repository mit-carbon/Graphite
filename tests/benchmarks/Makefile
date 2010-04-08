SIM_ROOT ?= $(CURDIR)/../../
TEST_BENCH_DIR = $(SIM_ROOT)/tests/benchmarks/

TEST_BENCH_LIST = barnes_bench_test fmm_bench_test ocean_contiguous_bench_test ocean_non_contiguous_bench_test water-nsquared_bench_test water-spatial_bench_test cholesky_bench_test fft_bench_test lu_contiguous_bench_test lu_non_contiguous_bench_test radix_bench_test volrend_bench_test raytrace_bench_test radiosity_bench_test

regress_bench: $(TEST_BENCH_LIST)

clean:
	for t in $(patsubst %_bench_test,%,$(TEST_BENCH_LIST)) ; do make -C $(TEST_BENCH_DIR)/$$t clean ; done

%_bench_test:
	$(MAKE) -C $(TEST_BENCH_DIR)/$(patsubst %_bench_test,%,$@)
