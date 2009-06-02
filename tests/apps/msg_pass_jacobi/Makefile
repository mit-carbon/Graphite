TARGET=jacobi
SOURCES = jacobi.cc

SIZE ?= 4
APP_FLAGS = -m $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
