TARGET=jacobi
SOURCES = jacobi.cc

SIZE ?= 4
APP_FLAGS ?= -m $(SIZE)
CORES ?= $(shell echo $$(( $(SIZE) + 1 )))
CLOCK_SKEW_MANAGEMENT_SCHEME ?= lax_p2p

include ../../Makefile.tests
