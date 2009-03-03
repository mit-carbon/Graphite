include ../makefile.gnu.config

DBG=-g
OPT=-O2
CXXFLAGS=-c -I../transport -I../system -I../core -I. -I../network -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = utils.cc config.cc bit_vector.cc packetize.cc basic_hash.cc lockfree_hash.cc locked_hash.cc semaphore.cc cond.cc log.cc lock.cc
OBJECTS = $(SOURCES:%.cc=%.o)

include ../makefile.common

