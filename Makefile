SIM_ROOT ?= $(CURDIR)

MPDS ?= 1

CLEAN=$(findstring clean,$(MAKECMDGOALS))

LIB_CARBON=$(SIM_ROOT)/lib/libcarbon_sim.a
LIB_PIN_SIM=$(SIM_ROOT)/pin/../lib/pin_sim.so

all: output_files $(LIB_CARBON) $(LIB_PIN_SIM)

include common/Makefile
include tests/apps/Makefile
include tests/unit/Makefile
include tests/benchmarks/Makefile

.PHONY: $(LIB_PIN_SIM)
$(LIB_PIN_SIM):
	$(MAKE) -C $(SIM_ROOT)/pin $@

clean: empty_logs
	$(MAKE) -C pin clean
	$(MAKE) -C common clean
	$(MAKE) -C tests/unit clean
	$(MAKE) -C tests/apps clean
	$(MAKE) -C tests/benchmarks clean

regress_quick: output_files regress_unit regress_apps

output_files:
	mkdir output_files

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

