
# Define the directories that will be compiled
DIRS_TO_COMPILE := util tech io \
                   model model/timing_graph \
                   model/std_cells \
                   model/electrical \
                   model/electrical/router \
                   model/optical \
                   model/optical_graph \
                   model/network \
                   model/network/ATAC

DIRS = $(patsubst %,$(CURDIR)/%,$(DIRS_TO_COMPILE))

SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))

OBJS = $(SRCS:%.cc=%.lo)

DEF_FLAGS = 
INCLUDE_FLAGS = -I$(CURDIR)
OPT_FLAGS = -O2 -g -fPIC
WARN_FLAGS = -pedantic -Wall -W #-Wextra -Werror -Wno-write-strings
CXXFLAGS = $(OPT_FLAGS) $(WARN_FLAGS) $(INCLUDE_FLAGS) $(DEF_FLAGS)

LD_LIBS += -lutil
LD_FLAGS += -Llibutil

# Other libraries used 
LIB_UTIL = $(CURDIR)/libutil/libutil.la

TARGET_LIB = $(CURDIR)/libdsent.la
TARGET_EXE = $(CURDIR)/dsent

all: $(TARGET_EXE)

$(TARGET_EXE): main.lo DSENT.lo $(LIB_UTIL) $(OBJS) 
	libtool --mode=link $(CXX) $(CXXFLAGS) $(LD_FLAGS) $(OBJS) main.lo DSENT.lo -o $(TARGET_EXE) $(LD_LIBS) 

static_lib: DSENT.lo $(LIB_UTIL) $(OBJS)
	libtool --mode=link $(CXX) -static $(CXXFLAGS) -o $(TARGET_LIB) DSENT.lo $(OBJS) $(LIB_UTIL)

# For general c++ compilation
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.lo: %.cc
	libtool --mode=compile $(CXX) -static -no-suppress $(CXXFLAGS) -c $< -o $@

$(LIB_UTIL):
	$(MAKE) -C $(CURDIR)/libutil static_lib TARGET_LIB=$(LIB_UTIL)

%/created:
	mkdir -p $(dir $@)
	touch $@

.phony: clean
clean:
	libtool --mode=clean $(RM) -rf main.lo DSENT.lo $(OBJS) $(TARGET_LIB) $(TARGET_EXE)
	$(MAKE) -C $(CURDIR)/libutil clean

