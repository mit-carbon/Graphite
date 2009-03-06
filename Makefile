
SIM_ROOT ?= $(CURDIR)

# if we are running the 'clean' target
# then don't include other files (which define clean)
# we simply use or own clean defined below
CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include common/Makefile.common
include common/tests/Makefile
endif

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

cannon_unit_test: simlib
	$(MAKE) -C tests/unit/cannon
	./tests/unit/cannon/cannon -m 4 -s 4

spawn_unit_test: simlib
	$(MAKE) -C tests/unit/spawn
	./tests/unit/spawn/spawn

ifneq ($(CLEAN),)
clean:
	$(MAKE) -C pin clean
	$(MAKE) -C lib clean
	$(MAKE) -C common clean
	-rm -f output_files/*
endif

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

