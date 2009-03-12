SIM_ROOT ?= $(CURDIR)

MPDS=1
PROCS=2
CORES=4
..PHONY: MPDS
..PHONY: PROCS
..PHONY: CORES

CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include common/Makefile.common
include tests/apps/Makefile.apps
include tests/unit/Makefile.unit
endif

TOTAL_CORES := $(shell echo $$(( $(CORES) + 1 )))

all:
	$(MAKE) -C common
	$(MAKE) -C pin

tests_to_clean =  hello_world simple file_io ping_pong mutex barrier cannon_msg \
                  cannon simple_test_dist cannon_msg ring_msg_pass dynamic_threads \
                  spawn_join

clean: empty_logs
	$(MAKE) -C pin clean
	$(MAKE) -C common clean
	for t in $(tests_to_clean) ; do make -C tests/apps/$$t clean ; done

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

