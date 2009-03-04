include common/makefile.gnu.config

SIM_ROOT=$(PWD)

include common/tests/Makefile

PIN_BIN=$(PIN_HOME)/pin
PIN_TOOL=pin/bin/pin_sim
#PIN_RUN=$(MPI_DIR)/bin/mpirun -np 1 $(PIN_BIN) -pause_tool 20 -mt -t $(PIN_TOOL) 
PIN_RUN=$(MPI_DIR)/bin/mpirun -np 1 $(PIN_BIN) -mt -t $(PIN_TOOL) 
PIN_RUN_DIST=$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) 

TESTS_DIR=./common/tests

CORES=4
TOTAL_CORES := $(shell echo $$(( $(CORES) + 1 )))

..PHONY: cores
PROCESS=mpirun
..PHONY: process

all:
	$(MAKE) -C common 
	$(MAKE) -C pin
	$(MAKE) -C qemu

pinbin:
	$(MAKE) -C common
	$(MAKE) -C pin

qemubin:
	$(MAKE) -C common
	$(MAKE) -C qemu

clean:
	$(MAKE) -C common clean
	$(MAKE) -C pin clean
	$(MAKE) -C qemu clean
	-rm -f *.o *.d *.rpo output_files/*

squeaky: clean
	$(MAKE) -C common squeaky
	$(MAKE) -C pin squeaky
	$(MAKE) -C qemu squeaky
	-rm -f *~

empty_logs :
	rm output_files/* ; true

run_mpd:
	$(MPI_DIR)/bin/mpd

stop_mpd:
	$(MPI_DIR)/bin/mpdallexit


love:
	@echo "not war!"

out:
	@echo "I think we should just be friends..."

