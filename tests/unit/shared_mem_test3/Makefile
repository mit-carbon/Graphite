TARGET=shared_mem_test3
SOURCES = shared_mem_test3.cc

CORES ?= 64
ENABLE_SM ?= true
MODE ?= 
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
