SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))
include $(SIM_ROOT)/common/Makefile.common

SRC_DIRECTORIES = $(DIRECTORIES) \
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_pr_l2_dram_directory_msi/				\
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_pr_l2_dram_directory_mosi/ 			\
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_sh_l2_msi/

# Grab all c/c++ files from subdirs
SOURCES = $(foreach dir,$(SRC_DIRECTORIES),$(wildcard $(dir)/*.cc)) \
	$(wildcard $(SIM_ROOT)/common/config/*.cpp)

# Convert the .c and .cc's in to .o's
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

# Other Libraries that are used by Graphite
DSENT_LIB = $(SIM_ROOT)/contrib/dsent/libdsent_contrib.a
MCPAT_LIB = $(SIM_ROOT)/contrib/mcpat/libmcpat.a
CONTRIB_LIBS = $(DSENT_LIB) $(MCPAT_LIB)

TARGET=$(SIM_ROOT)/lib/libcarbon_sim.a

# targets
all: $(TARGET)

$(TARGET): $(OBJECTS) $(CONTRIB_LIBS)
	ar rcs $@ $(OBJECTS)

.PHONY: $(CONTRIB_LIBS)

$(DSENT_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/dsent

$(MCPAT_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/mcpat

ifneq ($(CLEAN),)
clean:
	$(RM) $(TARGET) $(OBJECTS)
	make -C $(SIM_ROOT)/contrib/dsent clean
	make -C $(SIM_ROOT)/contrib/mcpat clean   
endif
