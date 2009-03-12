SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

# Grab all c/c++ files from subdirs
C_SOURCES = 
CPP_SOURCES =        		              \
          $(SIM_ROOT)/common/core/*.cc        \
          $(SIM_ROOT)/common/network/*.cc     \
          $(SIM_ROOT)/common/transport/*.cc   \
          $(SIM_ROOT)/common/misc/*.cc        \
          $(SIM_ROOT)/common/config/*.cpp     \
          $(SIM_ROOT)/common/system/*.cc      \
          $(SIM_ROOT)/common/user/*.cc

# Grab the actual filenames and convert them into .o files for dependencies
SOURCES = $(shell ls $(CPP_SOURCES) $(C_SOURCES))

# Convert the .c and .cc's in to .o's
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

TARGET=$(SIM_ROOT)/lib/libcarbon_sim.a

# targets
all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $^

ifneq ($(CLEAN),)
clean:
	-rm -f $(TARGET) $(OBJECTS)
endif
