SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))

include $(SIM_ROOT)/contrib/Makefile.common

$(DSENT_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/dsent

$(MCPAT_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/mcpat

ifneq ($(CLEAN),)
clean:
	make -C $(SIM_ROOT)/contrib/dsent clean
	make -C $(SIM_ROOT)/contrib/mcpat clean
endif
