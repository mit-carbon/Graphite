TARGET=shmem_perf_model
SOURCES = shmem_perf_model.c

CORES ?= 1
ENABLE_SM ?= true
MODE ?= 
APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/tile -I$(SIM_ROOT)/common/performance_model -I$(SIM_ROOT)/common/system -I$(SIM_ROOT)/common/config -I$(SIM_ROOT)/common/network -I$(SIM_ROOT)/common/transport -I$(SIM_ROOT)/common/tile/cache

include ../../Makefile.tests

