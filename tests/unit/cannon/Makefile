SOURCES = cannon.cc
TARGET = cannon

SIZE ?= 4
PROCS ?= 1
APP_FLAGS = -m $(SIZE) -s $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))
MODE ?= unit

include ../../Makefile.tests
