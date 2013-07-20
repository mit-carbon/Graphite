
# Collects the directories for sources to compile
DIRS_TO_COMPILE := .

DIRS = $(patsubst %,$(CURDIR)/%,$(DIRS_TO_COMPILE))
SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
OBJS = $(SRCS:%.cc=%.o)

INC_FLAGS   = -I$(CURDIR)
DEBUG_FLAGS = -g
OPT_FLAGS   = -O2 -fPIC
WARN_FLAGS  = -pedantic -Wall -W
CXX_FLAGS   = $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)

LD_LIBS     = 
LD_FLAGS    = 

TARGET = $(CURDIR)/libdsent_contrib.a

ALL_OBJS = $(shell find $(CURDIR) -type f -name '*.o')

.PHONY: all dsent-core clean

all: $(TARGET)

$(TARGET): $(OBJS) dsent-core
	@echo $(ALL_OBJS)
	$(AR) rcs $@ $(ALL_OBJS)

dsent-core:
	$(MAKE) -C $@ object

# For general c++ compilation
%.o: %.cc
	$(CXX) $(CXX_FLAGS) -c $< -o $@

%/created:
	mkdir -p $(dir $@)
	touch $@

clean:
	-$(MAKE) -C dsent-core clean
	-$(RM) $(OBJS) $(TARGET)

