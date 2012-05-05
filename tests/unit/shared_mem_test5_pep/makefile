TARGET=shared_mem_test5
SOURCES = shared_mem_test5.cc

MODE ?=
SIM_FLAGS ?= "--general/num_processes=1 --general/total_cores=4 --general/enable_shared_mem=true --caching_protocol/type=pr_l1_pr_l1_pr_l2_dram_directory_msi --l1_dcache/cache_size=1 --l1_dcache/cache_block_size=64 --l1_dcache/associativity=1 --l2_cache/cache_size=1 --l2_cache/cache_block_size=64 --l2_cache/associativity=1 --dram_directory/max_hw_sharers=1 --dram_directory/directory_type=full_map"

APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/tile -I$(SIM_ROOT)/common/tile/memory_subsystem -I$(SIM_ROOT)/common/tile/memory_subsystem/cache -I$(SIM_ROOT)/common/tile/memory_subsystem/directory_schemes -I$(SIM_ROOT)/common/performance_model -I$(SIM_ROOT)/common/system -I$(SIM_ROOT)/common/config -I$(SIM_ROOT)/common/network -I$(SIM_ROOT)/common/network/models -I$(SIM_ROOT)/common/transport -I$(SIM_ROOT)/os-services-25032-gcc.4.0.0-linux-ia32_intel64

include ../../Makefile.tests
