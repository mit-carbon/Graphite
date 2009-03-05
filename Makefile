
SIM_ROOT=$(PWD)

include common/makefile.common
include common/tests/Makefile


TESTS_DIR=./common/tests

CORES=4
TOTAL_CORES := $(shell echo $$(( $(CORES) + 1 )))

..PHONY: cores
PROCESS=mpirun
..PHONY: process

all:
	$(MAKE) -C common/user
	$(MAKE) -C lib
	$(MAKE) -C pin/src

simlib:
	$(MAKE) -C lib

spawn_unit_test: simlib
	$(MAKE) -C tests/unit/spawn
	./tests/unit/spawn/spawn

clean:
	$(MAKE) -C pin clean
	$(MAKE) -C lib clean
	$(MAKE) -C common clean
	-rm -f output_files/*

empty_logs :
	rm output_files/* ; true

run_mpd:
	$(MPI_DIR)/bin/mpd

stop_mpd:
	$(MPI_DIR)/bin/mpdallexit


love:
	@echo "not war!"

out:
	@echo "I think we should just be friends..."

