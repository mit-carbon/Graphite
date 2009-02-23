include ../makefile.gnu.config

DBG=-g
OPT=-O2
CXXFLAGS=-c -I../core -I../system -I../network -I../misc -I../transport -I../user -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
LDFLAGS=

SOURCES = syscall_server.cc mcp.cc sync_client.cc sync_server.cc network_model_analytical_server.cc lcp.cc simulator.cc sim_thread.cc core_manager.cc sim_thread_manager.cc
OBJECTS = $(SOURCES:%.cc=%.o)

include ../makefile.common

