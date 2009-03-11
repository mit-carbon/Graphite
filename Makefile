
SIM_ROOT ?= $(CURDIR)

PROCS=1
CORES=4
..PHONY: CORES
..PHONY: PROCS

# if we are running the 'clean' target
# then don't include other files (which define clean)
# we simply use or own clean defined below
CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include common/Makefile.common
include tests/apps/Makefile.apps
include tests/unit/Makefile.unit
endif

TOTAL_CORES := $(shell echo $$(( $(CORES) + 1 )))

all:
	$(MAKE) -C common/user
	$(MAKE) -C lib
	$(MAKE) -C pin/src

simlib:
	$(MAKE) -C lib

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
	$(MPI_DIR)/bin/mpdboot -n $(PROCS)

stop_mpd:
	$(MPI_DIR)/bin/mpdallexit

love:
	@echo "not war!"

out:
	@echo "I think we should just be friends..."

