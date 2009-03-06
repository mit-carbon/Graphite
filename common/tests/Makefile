

SIM_ROOT ?= $(CURDIR)/../../

TMP=$(shell uname -m)
ifeq (${TMP},x86_64)
  PIN_HOME ?= /afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux
else
  PIN_HOME ?= /afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux
endif
PIN_BIN=$(PIN_HOME)/pin
PIN_TOOL=pin/bin/pin_sim
PIN_RUN=$(MPI_DIR)/bin/mpirun -np 1 $(PIN_BIN) -mt -t $(PIN_TOOL) 
PIN_RUN_DIST=$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) 

regress_quick: clean simple_test io_test ping_pong_test mutex_test barrier_test cannon_msg cannon simple_test_dist cannon_msg_dist ring_msg_pass dynamic_threads spawn_join

regress: regress_quick clean_benchmarks build_benchmarks 1djacobi_test_quick 

clean:
	$(MAKE) -C shared_mem_test clean
	$(MAKE) -C ping_pong clean
	$(MAKE) -C capi_worker clean
	$(MAKE) -C pthreads_basic clean
	$(MAKE) -C shared_mem_jacobi clean
	rm -f *.o *.d *.rpo

clean_benchmarks:
	$(MAKE) -C fmm clean
	$(MAKE) -C barnes clean
	$(MAKE) -C radiosity clean
	$(MAKE) -C raytrace clean
	$(MAKE) -C volrend clean
	$(MAKE) -C ocean_contig clean
	$(MAKE) -C water-nsquared clean
	$(MAKE) -C water-spatial clean

build_benchmarks:
	$(MAKE) -C fmm
	$(MAKE) -C barnes
	$(MAKE) -C radiosity
	$(MAKE) -C ocean_contig
	$(MAKE) -C raytrace
	$(MAKE) -C volrend
	$(MAKE) -C water-nsquared
	$(MAKE) -C water-spatial


simple_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/simple
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 2 -- $(TESTS_DIR)/simple/simple_test

spawn_join: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/$@
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 3 -- $(TESTS_DIR)/$@/$@

dynamic_threads: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/dynamic_threads
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 2 -- $(TESTS_DIR)/dynamic_threads/dynamic_threads

simple_test_dist: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/simple_test_dist
	$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) -mdc -mpf -msys -np 2 -tc 3 -- $(TESTS_DIR)/simple_test_dist/simple_test_dist

cannon_msg_dist: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/cannon_msg
	$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) -mdc -mpf -msys -np 2 -tc $(TOTAL_CORES) -- $(TESTS_DIR)/cannon_msg/cannon -m $(CORES) -s $(CORES)

ring_msg_pass: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/ring_msg_pass
	$(MPI_DIR)/bin/mpirun -np $(TOTAL_CORES) $(PIN_BIN) -mt -t $(PIN_TOOL) -mdc -mpf -msys -np $(TOTAL_CORES) -tc $(TOTAL_CORES) -- $(TESTS_DIR)/ring_msg_pass/ring -m $(CORES)

io_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/file_io
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 3 -- $(TESTS_DIR)/file_io/file_io

ping_pong_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/ping_pong
	$(PIN_RUN) -mdc -msm -msys -mpf -np 1 -tc 3 -- $(TESTS_DIR)/ping_pong/ping_pong

ping_pong_dist: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/ping_pong
	$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) -mdc -mpf -msys -np 2 -tc 3 -- $(TESTS_DIR)/ping_pong/ping_pong

cannon: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/cannon
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc $(TOTAL_CORES) -- $(TESTS_DIR)/cannon/cannon -m $(CORES) -s $(CORES)

cannon_msg: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/cannon_msg
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 5 -- $(TESTS_DIR)/cannon_msg/cannon -m 4 -s 4

capi_worker: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/capi_worker
	$(PIN_RUN) -mdc -mpf -msys -n 8 -- $(TESTS_DIR)/capi_worker/capi_worker

shmem_test_new: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/shared_mem_test
#	$(PIN_RUN) -mdc -msm -n 2 -- $(TESTS_DIR)/shared_mem_test/test
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TESTS_DIR)/shared_mem_test/test_new 5

shmem_test_evic: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/shared_mem_test
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TESTS_DIR)/shared_mem_test/test_evic 5

1djacobi_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/1d_jacobi
	$(PIN_RUN) -mdc -msm -msys -mpf -np 1 -tc $(CORES) -- $(TESTS_DIR)/1d_jacobi/jacobi $(CORES) 64

1djacobi_test_quick: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/1d_jacobi
	$(PIN_RUN) -mdc -msm -msys -mpf -np 1 -tc 4 -- $(TESTS_DIR)/1d_jacobi/jacobi 4 64

jacobi_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/shared_mem_jacobi
	$(PIN_RUN) -mdc -msm -msys -mpf -n $(CORES) -dms 1000 -- $(TESTS_DIR)/shared_mem_jacobi/jacobi -n $(CORES)

mutex_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/mutex
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 2 -- $(TESTS_DIR)/mutex/mutex_test

cond_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/cond
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/cond/cond_test

broadcast_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/broadcast
	$(PIN_RUN) -mdc -mpf -msys -n 5 -- $(TESTS_DIR)/broadcast/broadcast_test

barrier_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/barrier
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 6 -- $(TESTS_DIR)/barrier/barrier_test



basic_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/pthreads_basic
	$(PIN_RUN) -mpf -mdc -msm -msys -n $(CORES) -dms 4 -- $(TESTS_DIR)/pthreads_basic/basic -n $(CORES)

cache_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/cache_model
	$(PIN_RUN) -mdc -mpf -n 2 -- $(TESTS_DIR)/cache_model/cache_test

# below here is the splash benchmarks
fmm_test: all empty_logs
	# note, the 5th line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TESTS_DIR)/fmm
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 11 -- $(TESTS_DIR)/fmm/FMM < $(TESTS_DIR)/fmm/inputs/input.256

barnes_test: all empty_logs
	# note, the last line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TESTS_DIR)/barnes
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 5 -- $(TESTS_DIR)/barnes/BARNES < $(TESTS_DIR)/barnes/input


radiosity_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/radiosity
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 10 -- $(TESTS_DIR)/radiosity/RADIOSITY -p 8 -batch -room

ocean_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/ocean_contig
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 9 -- $(TESTS_DIR)/ocean_contig/OCEAN -p8

raytrace_test: all empty_logs
	#FIXME has some build issues
	$(MAKE) -C $(TESTS_DIR)/raytrace
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 9 -- $(TESTS_DIR)/raytrace/RAYTRACE -p8 $(TESTS_DIR)/raytrace/inputs/teapot.env

volrend_test: all empty_logs
	#FIXME this one runs out of memory
	$(MAKE) -C $(TESTS_DIR)/volrend
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 45 -- $(TESTS_DIR)/volrend/VOLREND 8 $(TESTS_DIR)/volrend/inputs/head-scaleddown2

watern_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/water-nsquared
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 9 -- $(TESTS_DIR)/water-nsquared/WATER-NSQUARED < $(TESTS_DIR)/water-nsquared/input

waters_test: all empty_logs
	$(MAKE) -C $(TESTS_DIR)/water-spatial
	$(PIN_RUN) -mdc -mpf -msys -np 1 -tc 9 -- $(TESTS_DIR)/water-spatial/WATER-SPATIAL < $(TESTS_DIR)/water-spatial/input
