
# Define the directories that will be compiled
DIRS_TO_COMPILE := .

DIRS = $(patsubst %,$(CURDIR)/%,$(DIRS_TO_COMPILE))

SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))

OBJS = $(SRCS:%.cc=%.lo)

DEF_FLAGS = 
INCLUDE_FLAGS = -I$(CURDIR)
OPT_FLAGS = -O2 -g -fPIC
WARN_FLAGS = -pedantic -Wall -W #-Wextra -Werror -Wno-write-strings
CXXFLAGS = $(OPT_FLAGS) $(WARN_FLAGS) $(INCLUDE_FLAGS) $(DEF_FLAGS)

LD_LIBS += -ldsent
LD_FLAGS += -Ldsent-core

# Other libraries used 
LIB_DSENT = $(CURDIR)/dsent-core/libdsent.la

TARGET_LIB = $(CURDIR)/libdsent_contrib.a

all: static_lib

static_lib: $(LIB_DSENT) $(OBJS)
	libtool --mode=link $(CXX) -static $(CXXFLAGS) -o $(TARGET_LIB) $(OBJS) $(LIB_DSENT)
    
# For general c++ compilation
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.lo: %.cc
	libtool --mode=compile $(CXX) -static -no-suppress $(CXXFLAGS) -c $< -o $@

$(LIB_DSENT):
	$(MAKE) -C $(CURDIR)/dsent-core static_lib TARGET_LIB=$(LIB_DSENT)

%/created:
	mkdir -p $(dir $@)
	touch $@

.phony: clean
clean:
	libtool --mode=clean $(RM) -rf $(OBJS) $(TARGET_LIB)
	$(MAKE) -C $(CURDIR)/dsent-core clean

