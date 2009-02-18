include ../../common/makefile.gnu.config

DBG=-g
OPT=-O2
CFLAGS= -c -I./ -Wall -Wno-unknown-pragmas $(DBG) $(OPT) -MMD

MPI_DIR=/afs/csail/group/carbon/tools/mpich2-1.0.8/install
export LD_LIBRARY_PATH=$(MPI_DIR)/lib

all : pin no-pin

no-pin : mpi-bug
	mpirun -np 2 ./mpi-bug
	@echo
	@echo
	@echo
	mpirun -np 1 ./mpi-bug

pin : mpi-bug-pin.so
	mpirun -np 2 $(PIN_HOME)/pin -mt -t ./mpi-bug-pin.so -- ls
	@echo
	@echo
	@echo
	mpirun -np 1 $(PIN_HOME)/pin -mt -t ./mpi-bug-pin -- ls

mpi-bug : mpi-bug.cc
	$(CXX) -o $@ $^

mpi-bug-pin.o : mpi-bug.cc
	$(CXX) -D USE_PIN $(CFLAGS) $(PIN_CXXFLAGS) -o $@ $<

mpi-bug-pin.so : mpi-bug-pin.o
	$(CXX) -D USE_PIN $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $^ $(PIN_LIBS) $(DBG)
#	$(CXX) -g -D USE_PIN $(CFLAGS) $(PIN_CXXFLAGS) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $< $(PIN_LIBS)

clean :
	rm mpi-bug mpi-bug-pin.so mpi-bug-pin.o
