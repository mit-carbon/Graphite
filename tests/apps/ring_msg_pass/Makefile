TARGET=ring
SOURCES = ring.c

SIZE ?= 2
APP_FLAGS ?= -m $(SIZE)
CORES ?= $(shell echo $$(( $(SIZE) + 1 )))
CLOCK_SKEW_MANAGEMENT_SCHEME ?= lax_p2p

include ../../Makefile.tests
