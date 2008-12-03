include common/makefile.gnu.config

PIN_BIN=/afs/csail.mit.edu/group/carbon/tools/pin/current/pin
PIN_TOOL=pin/bin/pin_sim
#PIN_RUN=mpirun -np 1 $(PIN_BIN) -pause_tool 20 -mt -t $(PIN_TOOL) 
PIN_RUN=mpirun -np 1 $(PIN_BIN) -mt -t $(PIN_TOOL) 
TESTS_DIR=./common/tests

CORES=9
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
	-rm -f *.o *.d *.rpo output_files/*

squeaky: clean
	$(MAKE) -C common squeaky
	$(MAKE) -C pin squeaky
	$(MAKE) -C qemu squeaky
	-rm -f *~

simple_test: all
	$(MAKE) -C $(TESTS_DIR)/simple
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/simple/simple_test

io_test: all
	$(MAKE) -C $(TESTS_DIR)/file_io
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/file_io/file_io

ping_pong_test: all
	$(MAKE) -C $(TESTS_DIR)/ping_pong
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong
#	$(PIN_RUN) -mdc -msys -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong
#	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong

matmult_test: all
	$(MAKE) -C $(TESTS_DIR)/pthreads_matmult
#	$(PIN_RUN) -mdc -msm -msys -n $(CORES) -- $(TESTS_DIR)/pthreads_matmult/cannon -m $(CORES) -s $(CORES)
	$(PIN_RUN) -mdc -mpf -msys -n $(CORES) -- $(TESTS_DIR)/pthreads_matmult/cannon -m $(CORES) -s $(CORES)

shmem_test_new: all
	$(MAKE) -C $(TESTS_DIR)/shared_mem_test
#	$(PIN_RUN) -mdc -msm -n 2 -- $(TESTS_DIR)/shared_mem_test/test
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TESTS_DIR)/shared_mem_test/test_new 5 

shmem_test_evic: all
	$(MAKE) -C $(TESTS_DIR)/shared_mem_test
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TESTS_DIR)/shared_mem_test/test_evic 5 

jacobi_test: all
	$(MAKE) -C $(TESTS_DIR)/shared_mem_jacobi
	$(PIN_RUN) -mdc -msm -msys -n $(CORES) -dms 1000 -- $(TESTS_DIR)/shared_mem_jacobi/jacobi -n $(CORES)

mutex_test: all
	$(MAKE) -C $(TESTS_DIR)/mutex
	$(PIN_RUN) -mdc -mpf -msys -n 1 -- $(TESTS_DIR)/mutex/mutex_test

cond_test: all
	$(MAKE) -C $(TESTS_DIR)/cond
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/cond/cond_test

broadcast_test: all
	$(MAKE) -C $(TESTS_DIR)/broadcast
	$(PIN_RUN) -mdc -mpf -msys -n 5 -- $(TESTS_DIR)/broadcast/broadcast_test

barrier_test: all
	$(MAKE) -C $(TESTS_DIR)/barrier
	$(PIN_RUN) -mdc -mpf -msys -n 5 -- $(TESTS_DIR)/barrier/barrier_test


fmm_test: all
	# note, the 5th line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TESTS_DIR)/fmm
	$(PIN_RUN) -mdc -mpf -msys -n 9 -- $(TESTS_DIR)/fmm/FMM < $(TESTS_DIR)/fmm/inputs/input.256

basic_test: all
	$(MAKE) -C $(TESTS_DIR)/pthreads_basic
	$(PIN_RUN) -mpf -mdc -msm -msys -n $(CORES) -dms 4 -- $(TESTS_DIR)/pthreads_basic/basic -n $(CORES)

cache_test: all
	$(MAKE) -C $(TESTS_DIR)/cache_model
	$(PIN_RUN) -mdc -mpf -n 2 -- $(TESTS_DIR)/cache_model/cache_test

# below here is the splash benchmarks

barnes_test: all
	# note, the last line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TESTS_DIR)/barnes
	$(PIN_RUN) -mdc -mpf -msys -n 5 -- $(TESTS_DIR)/barnes/BARNES < $(TESTS_DIR)/barnes/input

war:	kill

kill:
	@echo "Killing All Possible Processes"
	killall -s 9 $(PROCESS)
	killall -s 9 ping_pong
	killall -s 9 test_new
	killall -s 9 test_evic
	killall -s 9 basic
	killall -s 9 jacobi
	killall -s 9 test

radiosity_test: all
	$(MAKE) -C $(TESTS_DIR)/radiosity
	$(PIN_RUN) -mdc -mpf -msys -n 10 -- $(TESTS_DIR)/radiosity/RADIOSITY -p 8 -batch -room

ocean_test: all
	$(MAKE) -C $(TESTS_DIR)/ocean_contig
	$(PIN_RUN) -mdc -mpf -msys -n 9 -- $(TESTS_DIR)/ocean_contig/OCEAN -p8

raytrace_test: all
	$(MAKE) -C $(TESTS_DIR)/raytrace
	$(PIN_RUN) -mdc -mpf -msys -n 9 -- $(TESTS_DIR)/raytrace/RAYTRACE -p8 $(TESTS_DIR)/raytrace/inputs/teapot.env

volrend_test: all
	$(MAKE) -C $(TESTS_DIR)/volrend
	$(PIN_RUN) -mdc -mpf -msys -n 45 -- $(TESTS_DIR)/volrend/VOLREND 8 $(TESTS_DIR)/volrend/inputs/head-scaleddown2

watern_test: all
	$(MAKE) -C $(TESTS_DIR)/water-nsquared
	$(PIN_RUN) -mdc -mpf -msys -n 9 -- $(TESTS_DIR)/water-nsquared/WATER-NSQUARED < $(TESTS_DIR)/water-nsquared/input

waters_test: all
	$(MAKE) -C $(TESTS_DIR)/water-spatial
	$(PIN_RUN) -mdc -mpf -msys -n 9 -- $(TESTS_DIR)/water-spatial/WATER-SPATIAL < $(TESTS_DIR)/water-spatial/input

love:
	@echo "not war!"

out:
	@echo "I think we should just be friends..."

