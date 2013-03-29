TARGET = frequency_scaling_random
SOURCES = frequency_scaling_random.cc

THREADS ?= 64
ITERATIONS ?= 50
MIN_FREQUENCY ?= 1.0
MAX_FREQUENCY ?= 3.0
APP_FLAGS ?= $(THREADS) $(ITERATIONS) $(MIN_FREQUENCY) $(MAX_FREQUENCY)
CORES ?= $(THREADS)

include ../../Makefile.tests
