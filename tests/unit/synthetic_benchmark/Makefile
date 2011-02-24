TARGET=synthetic_benchmark
SOURCES = synthetic_benchmark.cc

APP_FLAGS ?= "< ./tests/unit/synthetic_benchmark/inputs/input.64"

MODE ?=

APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/tile -I$(SIM_ROOT)/common/system -I$(SIM_ROOT)/common/tile/memory_subsystem -I$(SIM_ROOT)/common/performance_model -I$(SIM_ROOT)/common/network -I$(SIM_ROOT)/common/config 

include ../../Makefile.tests
