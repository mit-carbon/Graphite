TARGET = synthetic_network
SOURCES = synthetic_network.cc

MODE ?= 
APP_SPECIFIC_CXX_FLAGS ?= -I$(SIM_ROOT)/common/tile \
								  -I$(SIM_ROOT)/common/tile/core \
								  -I$(SIM_ROOT)/common/tile/memory_subsystem \
								  -I$(SIM_ROOT)/common/tile/memory_subsystem/performance_models \
								  -I$(SIM_ROOT)/common/network \
								  -I$(SIM_ROOT)/common/network/models \
								  -I$(SIM_ROOT)/common/transport \
								  -I$(SIM_ROOT)/common/system \
								  -I$(SIM_ROOT)/common/config

include ../../Makefile.tests
