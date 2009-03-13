SIM_ROOT ?= $(CURDIR)

MPDS=1
PROCS=2
CORES=4
..PHONY: MPDS
..PHONY: PROCS
..PHONY: CORES

CLEAN=$(findstring clean,$(MAKECMDGOALS))
include common/Makefile
include tests/apps/Makefile
include tests/unit/Makefile

TOTAL_CORES := $(shell echo $$(( $(CORES) + 1 )))

all:
	$(MAKE) -C common
	$(MAKE) -C pin

clean: empty_logs
	$(MAKE) -C pin clean
	$(MAKE) -C common clean
	$(MAKE) -C tests/unit clean
	$(MAKE) -C tests/apps clean

regress_quick: clean $(TEST_APP_LIST) $(TEST_UNIT_LIST)

empty_logs :
	rm output_files/* ; true

run_mpd:
	$(MPI_DIR)/bin/mpdboot -n $(MPDS)

stop_mpd:
	$(MPI_DIR)/bin/mpdallexit

love:
	@echo "not war!"

out:
	@echo "I think we should just be friends..."

