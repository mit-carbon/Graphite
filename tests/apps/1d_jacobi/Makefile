TARGET=jacobi
SOURCES = jacobi.c

SIZE ?= 4
APP_FLAGS ?= $(SIZE) 64
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
