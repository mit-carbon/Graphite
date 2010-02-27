TARGET=shared_mem_test1
SOURCES = shared_mem_test1.cc

CORES ?= 2
ENABLE_SM ?= true
MODE ?= 
APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/core -I$(SIM_ROOT)/common/core/memory_subsystem -I$(SIM_ROOT)/common/core/memory_subsystem/cache -I$(SIM_ROOT)/common/performance_model -I$(SIM_ROOT)/common/system -I$(SIM_ROOT)/common/config -I$(SIM_ROOT)/common/network -I$(SIM_ROOT)/common/transport -I$(SIM_ROOT)/os-services-25032-gcc.4.0.0-linux-ia32_intel64

include ../../Makefile.tests
