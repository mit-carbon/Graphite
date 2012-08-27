SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))
include $(SIM_ROOT)/common/Makefile.common

SRC_DIRECTORIES = $(DIRECTORIES) \
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_pr_l2_dram_directory_msi/				\
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_pr_l2_dram_directory_mosi/ 			\
			 $(SIM_ROOT)/common/tile/memory_subsystem/sh_l1_sh_l2/										\
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_sh_l2_msi/

# Grab all c/c++ files from subdirs
SOURCES = $(foreach dir,$(SRC_DIRECTORIES),$(wildcard $(dir)/*.cc)) \
	$(wildcard $(SIM_ROOT)/common/config/*.cpp)

# Convert the .c and .cc's in to .o's
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

# Other Libraries that are used by Graphite (Only Orion right now)
ORION_LIB = $(SIM_ROOT)/contrib/orion/liborion.a
CONTRIB_LIBS = $(ORION_LIB)

TARGET=$(SIM_ROOT)/lib/libcarbon_sim.a

# targets
all: $(TARGET)

$(TARGET): $(OBJECTS) $(CONTRIB_LIBS)
	ar rcs $@ $(OBJECTS)

$(ORION_LIB):
	$(MAKE) -C $(SIM_ROOT)/contrib/orion

ifneq ($(CLEAN),)
clean:
	-rm -f $(TARGET) $(OBJECTS)
	make -C $(SIM_ROOT)/contrib/orion clean
endif
