SIM_ROOT ?= $(CURDIR)

CLEAN=$(findstring clean,$(MAKECMDGOALS))

LIB_CARBON=$(SIM_ROOT)/lib/libcarbon_sim.a
LIB_PIN_SIM=$(SIM_ROOT)/pin/../lib/pin_sim.so

all: output_files $(LIB_CARBON) $(LIB_PIN_SIM)

include common/Makefile
include tests/apps/Makefile
include tests/unit/Makefile
include tests/benchmarks/Makefile
ifneq ($(findstring parsec,$(MAKECMDGOALS)),)
include tests/Makefile.parsec
endif

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
