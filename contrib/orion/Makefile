SIM_ROOT ?= $(CURDIR)/../../

include $(SIM_ROOT)/Makefile.config

BUFFER_DIR = Buffer
CROSSBAR_DIR = Crossbar
ALLOCATOR_DIR = Allocator

DIRS = \
  $(CURDIR) \
  $(CURDIR)/$(BUFFER_DIR) \
  $(CURDIR)/$(CROSSBAR_DIR) \
  $(CURDIR)/$(ALLOCATOR_DIR)

SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))

OBJS = $(SRCS:%.cc=%.o)

INCLUDE_FLAGS = $(foreach dir, $(DIRS), -I$(dir))
OPT_FLAGS = -O2 -g
WARN_FLAGS = -pedantic -Wall -W -Wextra -Werror
CXXFLAGS = $(OPT_FLAGS) $(WARN_FLAGS) $(INCLUDE_FLAGS)
CXXFLAGS += -fPIC

TARGET = $(CURDIR)/liborion.a

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

%/created:
	mkdir -p $(dir $@)
	touch $@

clean:
	$(RM) -rf $(OBJS) *-test.o $(TARGET)
