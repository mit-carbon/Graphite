SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))
include $(SIM_ROOT)/common/Makefile.common

SRC_DIRECTORIES = $(DIRECTORIES) \
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_pr_l2_dram_directory_msi/				\
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_pr_l2_dram_directory_mosi/ 			\
			 $(SIM_ROOT)/common/tile/memory_subsystem/pr_l1_sh_l2_msi/

# Grab all c/c++ files from subdirs
SOURCES = $(foreach dir,$(SRC_DIRECTORIES),$(wildcard $(dir)*.cc)) \
	$(wildcard $(SIM_ROOT)/common/config/*.cpp)

# Convert the .c, .cc and .cpp's into .o's
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

# Get the dependencies from the objects
DEPENDENCIES = $(OBJECTS:%.o=%.d)

TARGET=$(SIM_ROOT)/lib/libcarbon_sim.a

# targets
all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

ifeq ($(CLEAN),)
-include $(OBJECTS:%.o=%.d)
endif

ifneq ($(CLEAN),)
clean:
	$(RM) $(TARGET) $(OBJECTS) $(DEPENDENCIES)
endif
