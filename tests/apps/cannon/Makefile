TARGET=cannon
SOURCES = cannon.c

SIZE ?= 4
APP_FLAGS ?= -m $(SIZE) -s $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
