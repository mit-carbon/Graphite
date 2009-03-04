
ifeq ($(SIM_ROOT),)
   # SIM_ROOT=$(PWD)/../
   SIM_ROOT=$(PWD)/
endif

OS_SERVICES_ROOT = $(SIM_ROOT)/os-services-25032-gcc.4.0.0-linux-ia32_intel64
BOOST_ROOT = /afs/csail/group/carbon/tools/boost_1_38_0

# Grab all c/c++ files from subdirs
C_SOURCES = ../common/user/*.c
CPP_SOURCES =                        \
          ../common/core/*.cc        \
          ../common/network/*.cc     \
          ../common/transport/*.cc   \
          ../common/misc/*.cc        \
          ../common/config/*.cpp     \
          ../common/system/*.cc

# Grab the actual filenames and convert them into .o files for dependencies
SOURCES = $(shell ls $(CPP_SOURCES) $(C_SOURCES))

# Convert the .c and .cc's in to .o's
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(SOURCES) ) ) )

.SUFFIXES:  .o .c .h .cc

CXXFLAGS+=-c -I$(SIM_ROOT)/common/user      \
          -I$(SIM_ROOT)/common/system       \
          -I$(SIM_ROOT)/common/core         \
          -I$(SIM_ROOT)/common/network      \
          -I$(SIM_ROOT)/common/transport    \
          -I$(SIM_ROOT)/common/misc         \
          -I$(SIM_ROOT)/common/config       \
          -I /afs/csail/group/carbon/tools/mpich2-1.0.8/install/include/ \
          -I/afs/csail/group/carbon/tools/boost_1_38_0/                  \
          -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD -m32 -ggdb

CXXFLAGS += -DSIM_STANDALONE

OS_SERVICES_ROOT = ../../../os-services-25032-gcc.4.0.0-linux-ia32_intel64

# Add support for os services
LD_LIBS += -los-services
ifeq ($(BUILD_32),1)
  CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-ia32
  LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/ia32 -m32
else
  CXXFLAGS+= -I$(OS_SERVICES_ROOT)/include-intel64
  LD_FLAGS += -los-services -L $(OS_SERVICES_ROOT)/intel64
endif

CXXFLAGS+=$(PIN_CXXFLAGS)
CFLAGS=$(CXXFLAGS)

include $(SIM_ROOT)/common/makefile.gnu.config

# build rules
%.d: %.cpp
	$(CXX) -MM -MG $(CXXFLAGS) $< | sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' >$@

%.d: %.cc
	$(CXX) -MM -MG $(CXXFLAGS) $< | sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' >$@

%.d: %.c
	$(CC) -MM -MG $(CFLAGS) $< | sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' >$@


# targets
all: libcarbon_sim.a


libcarbon_sim.a: $(OBJECTS)
	ar rcs $@ $^

show_home:
	@echo $(SIM_ROOT)

ifneq ($(MAKECMDGOALS),clean)
include $(OBJECTS:%.o=%.d)
endif

clean:
	-rm ./*.d ./*.o $(OBJECTS)


