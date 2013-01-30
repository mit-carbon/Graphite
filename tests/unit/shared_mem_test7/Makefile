TARGET=shared_mem_test7
SOURCES = shared_mem_test7.cc

MODE ?=
SIM_FLAGS ?= "--general/num_processes=1 --general/total_cores=4 --general/enable_shared_mem=true --caching_protocol/type=pr_l1_pr_l2_dram_directory_mosi --l1_dcache/T1/cache_size=1 --l1_dcache/T1/cache_block_size=64 --l1_dcache/T1/associativity=1 --l2_cache/T1/cache_size=1 --l2_cache/T1/cache_block_size=64 --l2_cache/T1/associativity=1 --dram_directory/max_hw_sharers=1 --dram_directory/directory_type=limited_broadcast"

APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/tile \
								  -I$(SIM_ROOT)/common/tile/core \
								  -I$(SIM_ROOT)/common/tile/core/performance_models \
								  -I$(SIM_ROOT)/common/tile/memory_subsystem \
								  -I$(SIM_ROOT)/common/tile/memory_subsystem/cache \
								  -I$(SIM_ROOT)/common/tile/memory_subsystem/performance_models \
								  -I$(SIM_ROOT)/common/system \
								  -I$(SIM_ROOT)/common/config \
								  -I$(SIM_ROOT)/common/network \
								  -I$(SIM_ROOT)/common/transport

include ../../Makefile.tests
