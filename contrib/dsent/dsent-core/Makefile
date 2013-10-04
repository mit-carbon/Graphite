# Define the directories that will be compiled
DIRS_TO_COMPILE := libutil \
                   util tech io \
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
OBJS = $(SRCS:%.cc=%.o) $(CURDIR)/DSENT.o

ifdef LIBUTIL_IS_LOG
	LIBUTIL_IS_LOG = true
else
	LIBUTIL_IS_LOG = false
endif
DEF_FLAGS += -DLIBUTIL_IS_LOG=$(LIBUTIL_IS_LOG)

INC_FLAGS   = -I$(CURDIR)
DEBUG_FLAGS = -g
OPT_FLAGS   = -O2 -fPIC
WARN_FLAGS  = -pedantic -Wall -W
CXX_FLAGS   = $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(INC_FLAGS) $(DEF_FLAGS)

LD_LIBS     =
LD_FLAGS    =

TARGET = dsent.exe

all: $(TARGET)

$(TARGET): main.o $(OBJS) 
	$(CXX) $(CXX_FLAGS) $(LD_FLAGS) $(OBJS) main.o -o $(TARGET) $(LD_LIBS) 

object: $(OBJS)

# For general c++ compilation
%.o: %.cc
	$(CXX) $(CXX_FLAGS) -c $< -o $@

%/created:
	mkdir -p $(dir $@)
	touch $@

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(TARGET)
