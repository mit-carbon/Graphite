TARGET=limited_dir
SOURCES = limited_dir.cc

SIM_FLAGS ?= "-c $(CURDIR)/../../../carbon_sim.cfg --general/num_processes=1 --general/total_cores=10 --general/enable_shared_mem=true --l2_cache/track_detailed_cache_counters=true"

MODE ?= 

APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/tile -I$(SIM_ROOT)/common/performance_model -I$(SIM_ROOT)/common/system -I$(SIM_ROOT)/common/config -I$(SIM_ROOT)/common/network -I$(SIM_ROOT)/common/transport -I$(SIM_ROOT)/common/tile/cache -I$(SIM_ROOT)/os-services-25032-gcc.4.0.0-linux-ia32_intel64

include ../../Makefile.tests
