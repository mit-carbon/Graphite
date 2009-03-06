
BUILD_32 = 1

SIM_ROOT ?= $(CURDIR)/../../..
TEST_SUPPORT_DIR = $(SIM_ROOT)/tests/unit/test_support

LD_FLAGS += -L$(SIM_ROOT)/lib 
LD_LIBS += -pthread -lcarbon_sim

SUPPORT_SOURCES = $(TEST_SUPPORT_DIR)/pthread_thread.cc $(TEST_SUPPORT_DIR)/test_support.cc

SUPPORT_OBJECTS = $(SUPPORT_SOURCES:%.cc=%.o)

CXXFLAGS = -I$(SIM_ROOT)/common -I$(SIM_ROOT)/tests/unit/test_support \
           -c -ggdb3 -Wall -m32

CFLAGS = $(CXXFLAGS)

all: $(TARGET)

CLEAN=$(findstring clean,$(MAKECMDGOALS))
ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

.SUFFIXES:  .o .c .h .cc

$(TARGET): $(OBJECTS) $(SUPPORT_OBJECTS) $(SIM_ROOT)/lib/libcarbon_sim.a
	$(CXX) $^ -o $@ $(LD_FLAGS) $(LD_LIBS) 

show_pinflags:
	@echo $(PIN_LDFLAGS)

ifneq ($(CLEAN),)
clean:
	-rm -f *.o *.d $(TARGET)
endif
