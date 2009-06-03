TARGET=jacobi
SOURCES = jacobi.c

SIZE ?= 4
APP_FLAGS ?= $(SIZE) 2
CORES = $(shell echo $$(( $(SIZE) + 1 )))
ENABLE_SM = true

include ../../Makefile.tests
