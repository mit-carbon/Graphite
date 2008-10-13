include common/makefile.gnu.config

PIN_BIN=/afs/csail.mit.edu/group/carbon/tools/pin/current/pin
PIN_TOOL=pin/bin/pin_sim
#PIN_RUN=mpirun -np 1 $(PIN_BIN) -pause_tool 20 -mt -t $(PIN_TOOL) 
PIN_RUN=mpirun -np 1 $(PIN_BIN) -mt -t $(PIN_TOOL) 
TESTS_DIR=./common/tests

CORES=16
..PHONY: cores
PROCESS=mpirun
..PHONY: process

all:
	$(MAKE) -C common 
	$(MAKE) -C pin
	$(MAKE) -C qemu

use-mpi:
	$(MAKE) use-mpi -C common/phys_trans
	$(MAKE) clean -C common/phys_trans

use-sm:
	$(MAKE) use-sm -C common/phys_trans
	$(MAKE) clean -C common/phys_trans

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
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C common squeaky
	$(MAKE) -C pin squeaky
	$(MAKE) -C qemu squeaky
	-rm -f *~

io_test: all
	$(MAKE) -C $(TESTS_DIR)/file_io
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/file_io/file_io

ping_pong_test: all
	$(MAKE) -C $(TESTS_DIR)/ping_pong
	$(PIN_RUN) -mdc -msm -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong
#	$(PIN_RUN) -mdc -msys -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong
#	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong

matmult_test: all
	$(MAKE) -C $(TESTS_DIR)/pthreads_matmult
	$(PIN_RUN) -mdc -mpf -msys -n $(CORES) -- $(TESTS_DIR)/pthreads_matmult/cannon -m $(CORES) -s $(CORES)

shmem_test: all
	$(MAKE) -C $(TESTS_DIR)/shared_mem_test
#	$(PIN_RUN) -mdc -msm -n 2 -- $(TESTS_DIR)/shared_mem_test/test
	$(PIN_RUN) -mdc -msm -msys -n 2 -- $(TESTS_DIR)/shared_mem_test/test

basic_test: all
	$(MAKE) -C $(TESTS_DIR)/pthreads_basic
	#$(PIN_RUN) -mdc -msm -msys -n 2 -- $(TESTS_DIR)/pthreads_basic/basic
	$(PIN_RUN) -mdc -msm -msys -n 1 -- $(TESTS_DIR)/pthreads_basic/basic

cache_test: all
	$(MAKE) -C $(TESTS_DIR)/cache_model
	$(PIN_RUN) -mdc -mpf -n 2 -- $(TESTS_DIR)/cache_model/cache_test

barnes_test: all
	$(MAKE) -C $(TESTS_DIR)/barnes
	$(PIN_RUN) -mdc -mpf -msys -n 4 -- $(TESTS_DIR)/barnes/BARNES < $(TESTS_DIR)/barnes/input

war:	kill
kill:
	@echo "Killing All Possible Processes"
	killall -s 9 $(PROCESS)
	killall -s 9 test
	killall -s 9 ping_pong
	killall -s 9 basic

love:
	@echo "not war!"

out:
	@echo "I think we should just be friends..."

