
BUILD_32 = 1

ifeq ($(SIM_ROOT),)
SIM_ROOT = $(CURDIR)/../../../
endif

LD_FLAGS += -L$(SIM_ROOT)/lib 
LD_LIBS += -pthread -lcarbon_sim

SUPPORT_SOURCES = $(SIM_ROOT)/pin/src/pin_thread.cc $(SIM_ROOT)/tests/unit/test_support/test_support.cc

SUPPORT_OBJECTS = $(SUPPORT_SOURCES:%.cc=%.o)

CXXFLAGS = -I$(SIM_ROOT)/common -I$(SIM_ROOT)/pin/src -I$(SIM_ROOT)/tests/unit/test_support \
           -c -ggdb -Wall -m32

CFLAGS = $(CXXFLAGS)

all: $(TARGET)

CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/makefile.common
endif

.SUFFIXES:  .o .c .h .cc

$(TARGET): $(OBJECTS) $(SUPPORT_OBJECTS)
	$(CXX) $^ -o $@ $(LD_FLAGS) $(LD_LIBS) 

show_pinflags:
	@echo $(PIN_LDFLAGS)

ifneq ($(CLEAN),)
clean:
	-rm -f *.o *.d $(TARGET)
endif
