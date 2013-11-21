
# Define the directories that will be compiled
DIRS_TO_COMPILE := .

DIRS = $(patsubst %,$(CURDIR)/%,$(DIRS_TO_COMPILE))
SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
OBJS = $(SRCS:%.cc=%.o)

DEF_FLAGS = 

ifdef LIBUTIL_IS_LOG
	LIBUTIL_IS_LOG = true
else
	LIBUTIL_IS_LOG = false
endif
DEF_FLAGS += -DLIBUTIL_IS_LOG=$(LIBUTIL_IS_LOG)

INC_FLAGS   = $(foreach dir, $(DIRS), -I$(dir))
DEBUG_FLAGS = -g
OPT_FLAGS   = -O2 -fPIC -lm
WARN_FLAGS  = -pedantic -Wall -W -Wextra #-Werror
CXX_FLAGS   = $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(INCLUDE_FLAGS) $(DEF_FLAGS)

TARGET = $(CURDIR)/libutil.a

.PHONY: all clean

#all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.cc
	$(CXX) $(CXX_FLAGS) -c $< -o $@

%/created:
	mkdir -p $(dir $@)
	touch $@

clean:
	-$(RM) -rf $(OBJS) $(TARGET)

