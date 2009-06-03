TARGET=ring
SOURCES = ring.c

SIZE ?= 2
APP_FLAGS = -m $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
