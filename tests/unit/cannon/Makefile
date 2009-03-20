SOURCES = cannon.cc
TARGET = cannon

SIZE ?= 4
APP_FLAGS = -m $(SIZE) -s $(SIZE)
CORES = $(shell echo $$(( $(SIZE) + 1 )))

include ../../Makefile.tests
