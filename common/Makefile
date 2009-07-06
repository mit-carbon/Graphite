SIM_ROOT ?= $(CURDIR)/../

CLEAN=$(findstring clean,$(MAKECMDGOALS))
include $(SIM_ROOT)/common/Makefile.common

# Grab all c/c++ files from subdirs
SOURCES = $(foreach dir,$(DIRECTORIES),$(wildcard $(dir)/*.cc)) \
	$(wildcard $(SIM_ROOT)/common/config/*.cpp)

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
