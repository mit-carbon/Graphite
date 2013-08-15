TARGET=libdb_utils.a

SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:%.cc=%.o)

CXXFLAGS= -c -O3 -Wall -Wno-unknown-pragmas -fPIC

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $^

%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJECTS) $(TARGET)
