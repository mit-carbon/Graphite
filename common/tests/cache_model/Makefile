include ../../makefile.gnu.config

CFLAGS += -O2 -I../../core -I../../misc -g -Wall -DPERFORMING_CACHE_TEST

## Build rules

all: cache_test

cache_test: cache_test.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -c -o utils.o ../../misc/utils.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -c -o cache.o ../../core/cache.cc
	$(CXX) $(CFLAGS) $(PIN_CXXFLAGS) -o cache_test cache_test.cc cache.o utils.o

## Cleaning

clean:
	-rm -f *.o cache_test

squeaky: clean
	-rm -f *~
