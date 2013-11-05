SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))

include $(SIM_ROOT)/contrib/Makefile.common

$(DSENT_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/dsent

$(MCPAT_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/mcpat USE_GRAPHITE=true

$(DB_UTILS_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/db_utils

ifneq ($(CLEAN),)
clean:
	make -C $(SIM_ROOT)/contrib/dsent clean
	make -C $(SIM_ROOT)/contrib/mcpat clean
	make -C $(SIM_ROOT)/contrib/db_utils clean
endif
