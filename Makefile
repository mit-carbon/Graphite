SIM_ROOT ?= $(CURDIR)

CLEAN=$(findstring clean,$(MAKECMDGOALS))

CARBON_LIB = $(SIM_ROOT)/lib/libcarbon_sim.a
DSENT_LIB = $(SIM_ROOT)/contrib/dsent/libdsent_contrib.a
MCPAT_LIB = $(SIM_ROOT)/contrib/mcpat/libmcpat.a
DB_UTILS_LIB = $(SIM_ROOT)/contrib/db_utils/libdb_utils.a
CONTRIB_LIBS = $(DSENT_LIB) $(MCPAT_LIB) $(DB_UTILS_LIB)
PIN_SIM_LIB = $(SIM_ROOT)/lib/pin_sim.so

all: $(CARBON_LIB) $(CONTRIB_LIBS) $(PIN_SIM_LIB)

include common/Makefile
include contrib/Makefile
include tests/apps/Makefile
include tests/unit/Makefile
include tests/benchmarks/Makefile
ifneq ($(findstring parsec,$(MAKECMDGOALS)),)
include tests/Makefile.parsec
endif

.PHONY: $(PIN_SIM_LIB)
$(PIN_SIM_LIB):
	$(MAKE) -C $(SIM_ROOT)/pin

clean:
	$(MAKE) -C pin clean
	$(MAKE) -C common clean
	$(MAKE) -C contrib clean
	$(MAKE) -C tests/unit clean
	$(MAKE) -C tests/apps clean
	$(MAKE) -C tests/benchmarks clean

clean_output_dirs:
	rm -f $(SIM_ROOT)/results/latest
	rm -rf $(SIM_ROOT)/results/[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]_[0-9][0-9]-[0-9][0-9]-[0-9][0-9]

regress_quick: regress_unit regress_apps
