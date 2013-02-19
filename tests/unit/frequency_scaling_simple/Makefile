TARGET = frequency_scaling_simple
SOURCES = frequency_scaling_simple.cc

THREADS ?= 64
ITERATIONS ?= 10
MIN_FREQUENCY ?= 0.5
MAX_FREQUENCY ?= 5.0
APP_FLAGS ?= $(THREADS) $(ITERATIONS) $(MIN_FREQUENCY) $(MAX_FREQUENCY)
CORES ?= $(THREADS)

include ../../Makefile.tests
