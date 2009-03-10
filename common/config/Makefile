

CONFIG_SOURCES = config.cpp config_file.cpp key.cpp section.cpp
CONFIG_OBJECTS = $(patsubst %.cpp,%.o,$(CONFIG_SOURCES))

.SUFFIXES: .c .cpp

CXXFLAGS := -I/afs/csail/group/carbon/tools/boost_1_38_0/

all: test

BOOST_ROOT = /afs/csail/group/carbon/tools/boost_1_38_0/build

BOOST_SUFFIX = gcc41-mt-1_38

LD_FLAGS += -L/$(BOOST_ROOT)
LD_LIBS += -lboost_filesystem-$(BOOST_SUFFIX) -lboost_system-$(BOOST_SUFFIX) -pthread 

test: $(CONFIG_OBJECTS) test.o
	$(CXX) $(LD_FLAGS) -o $@ $^ $(LD_LIBS) 





