TARGET=cannon
SOURCES = cannon.c

SIZE ?= 4
APP_FLAGS ?= -t $(SIZE) -m $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
