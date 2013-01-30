SIM_ROOT ?= $(CURDIR)

CLEAN=$(findstring clean,$(MAKECMDGOALS))

LIB_CARBON=$(SIM_ROOT)/lib/libcarbon_sim.a
LIB_PIN_SIM=$(SIM_ROOT)/pin/../lib/pin_sim.so

all: $(LIB_CARBON) $(LIB_PIN_SIM)

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

clean:
	$(MAKE) -C pin clean
	$(MAKE) -C common clean
	$(MAKE) -C tests/unit clean
	$(MAKE) -C tests/apps clean
	$(MAKE) -C tests/benchmarks clean

clean_output_dirs:
	rm -f $(SIM_ROOT)/results/latest
	rm -rf $(SIM_ROOT)/results/[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]_[0-9][0-9]-[0-9][0-9]-[0-9][0-9]

regress_quick: regress_unit regress_apps
