DIRECTORIES = \
				  $(CURDIR)					\
				  $(CURDIR)/Allocator	\
				  $(CURDIR)/Buffer		\
				  $(CURDIR)/Crossbar

# Grab all c++ files from subdirs
SOURCES = $(foreach dir,$(DIRECTORIES),$(wildcard $(dir)/*.cc))

# All Objects
OBJECTS = $(SOURCES:%.cc=%.o)

# Include paths
CXXFLAGS = -c $(foreach dir,$(DIRECTORIES),-I$(dir)) \
          -Wall -Werror -Wno-unknown-pragmas -O2 -fPIC

TARGET = $(CURDIR)/liborion.a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $^

%.o : %.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) $(OBJECTS)
