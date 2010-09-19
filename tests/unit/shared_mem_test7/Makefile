TARGET=shared_mem_test7
SOURCES = shared_mem_test7.cc

MODE ?=
SIM_FLAGS ?= "--general/num_processes=1 --general/total_cores=4 --general/enable_shared_mem=true --caching_protocol/type=pr_l1_pr_l2_dram_directory_mosi --perf_model/l1_dcache/T1/cache_size=1 --perf_model/l1_dcache/T1/cache_block_size=64 --perf_model/l1_dcache/T1/associativity=1 --perf_model/l2_cache/T1/cache_size=1 --perf_model/l2_cache/T1/cache_block_size=64 --perf_model/l2_cache/T1/associativity=1 --perf_model/dram_directory/max_hw_sharers=1 --perf_model/dram_directory/directory_type=limited_broadcast"

APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/core -I$(SIM_ROOT)/common/core/memory_subsystem -I$(SIM_ROOT)/common/core/memory_subsystem/cache -I$(SIM_ROOT)/common/core/memory_subsystem/directory_schemes -I$(SIM_ROOT)/common/performance_model -I$(SIM_ROOT)/common/performance_model/memory_subsystem -I$(SIM_ROOT)/common/system -I$(SIM_ROOT)/common/config -I$(SIM_ROOT)/common/network -I$(SIM_ROOT)/common/transport -I$(SIM_ROOT)/os-services-25032-gcc.4.0.0-linux-ia32_intel64

include ../../Makefile.tests
