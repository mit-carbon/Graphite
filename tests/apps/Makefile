SIM_ROOT ?= $(CURDIR)/../../
TEST_APPS_DIR = $(SIM_ROOT)/tests/apps/
CLEAN=$(findstring clean,$(MAKECMDGOALS))

TMP=$(shell uname -m)
ifeq (${TMP},x86_64)
  PIN_HOME ?= /afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux
else
  PIN_HOME ?= /afs/csail/group/carbon/tools/pin/pin-2.5-24110-gcc.4.0.0-ia32_intel64-linux
endif

PIN_HOME = /afs/csail/group/carbon/tools/pin/current

ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

PIN_BIN=$(PIN_HOME)/ia32/bin/pinbin
PIN_TOOL=$(SIM_ROOT)/lib/pin_sim
PIN_RUN=$(MPI_DIR)/bin/mpirun -np 1 $(PIN_BIN) -mt -t $(PIN_TOOL) 
PIN_RUN_DIST=$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) 

TEST_APP_LIST = hello_world_test simple_test file_io_test ping_pong_test mutex_test barrier_test cannon_msg_test \
                cannon_test simple_dist_test cannon_msg_dist_test ring_msg_pass_test dynamic_threads_test \
		spawn_join_test

#TEST_APP_DIRS = $(patsubst %_test,%,$(TEST_UNIT_LIST))
TEST_APP_DIRS = hello_world simple file_io ping_pong mutex barrier cannon_msg \
                cannon simple_dist ring_msg_pass dynamic_threads \
		spawn_join

clean_sim:
	$(MAKE) -C $(SIM_ROOT) clean

regress: regress_quick clean_benchmarks build_benchmarks 1djacobi_test_quick 

clean:
	for t in $(TEST_APP_DIRS) ; do make -C $(TEST_APPS_DIR)/$$t clean ; done

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

sim:
	$(MAKE) -C $(SIM_ROOT)/common
	$(MAKE) -C $(SIM_ROOT)/pin
	$(MAKE) -C $(SIM_ROOT) 

hello_world_test: sim
	$(MAKE) -C $(TEST_APPS_DIR)/hello_world
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=1 -- $(TEST_APPS_DIR)/hello_world/hello_world

simple_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/simple
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=2 -- $(TEST_APPS_DIR)/simple/simple_test

spawn_join_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/spawn_join
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=3 -- $(TEST_APPS_DIR)/spawn_join/spawn_join

dynamic_threads_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/dynamic_threads
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=2 -- $(TEST_APPS_DIR)/dynamic_threads/dynamic_threads

simple_dist_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/simple_dist
	$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL)  -c $(SIM_ROOT)/carbon_sim.cfg --general/num_processes=2 --general/total_cores=3 -- $(TEST_APPS_DIR)/simple_dist/simple_test_dist

cannon_msg_dist_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cannon_msg
	$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL)  -c $(SIM_ROOT)/carbon_sim.cfg --general/num_processes=2 --general/total_cores=$(TOTAL_CORES) -- $(TEST_APPS_DIR)/cannon_msg/cannon -m $(CORES) -s $(CORES)

ring_msg_pass_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ring_msg_pass
	$(MPI_DIR)/bin/mpirun -np 4 $(PIN_BIN) -mt -t $(PIN_TOOL)  -c $(SIM_ROOT)/carbon_sim.cfg --general/num_processes=4 --general/total_cores=$(TOTAL_CORES) -- $(TEST_APPS_DIR)/ring_msg_pass/ring -m $(CORES)

file_io_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/file_io
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=3 -- $(TEST_APPS_DIR)/file_io/file_io

ping_pong_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ping_pong
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/enable_shared_memory=true --general/total_cores=3 -- $(TEST_APPS_DIR)/ping_pong/ping_pong

ping_pong_dist_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ping_pong
	$(MPI_DIR)/bin/mpirun -np 2 $(PIN_BIN) -mt -t $(PIN_TOOL) -c $(SIM_ROOT)/carbon_sim.cfg --general/num_processes=2 --general/total_cores=3 -- $(TEST_APPS_DIR)/ping_pong/ping_pong

cannon_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cannon
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=$(TOTAL_CORES) -- $(TEST_APPS_DIR)/cannon/cannon -m $(CORES) -s $(CORES)

cannon_msg_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cannon_msg
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=5 -- $(TEST_APPS_DIR)/cannon_msg/cannon -m 4 -s 4

capi_worker_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/capi_worker
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg -- $(TEST_APPS_DIR)/capi_worker/capi_worker

shmem_test_new: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/shared_mem_test
#	$(PIN_RUN) -mdc -msm -n 2 -- $(TEST_APPS_DIR)/shared_mem_test/test
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TEST_APPS_DIR)/shared_mem_test/test_new 5

shmem_test_evic: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/shared_mem_test
	$(PIN_RUN) -mdc -msm -msys -mpf -n 2 -- $(TEST_APPS_DIR)/shared_mem_test/test_evic 5

1djacobi_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/1d_jacobi
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/total_cores=$(CORES) -- $(TEST_APPS_DIR)/1d_jacobi/jacobi $(CORES) 64

1djacobi_test_quick: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/1d_jacobi
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/enable_shared_memory=true --general/total_cores=4 -- $(TEST_APPS_DIR)/1d_jacobi/jacobi 4 64

jacobi_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/shared_mem_jacobi
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/enable_shared_memory=true -msm -msys -mpf -n $(CORES) -dms 1000 -- $(TEST_APPS_DIR)/shared_mem_jacobi/jacobi -n $(CORES)

mutex_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/mutex
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=2 -- $(TEST_APPS_DIR)/mutex/mutex_test

cond_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cond
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg -n 2 -- $(TEST_APPS_DIR)/cond/cond_test

broadcast_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/broadcast
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg -n 5 -- $(TEST_APPS_DIR)/broadcast/broadcast_test

barrier_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/barrier
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=6 -- $(TEST_APPS_DIR)/barrier/barrier_test



basic_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/pthreads_basic
	$(PIN_RUN) -mpf -mdc -msm -msys -n $(CORES) -dms 4 -- $(TEST_APPS_DIR)/pthreads_basic/basic -n $(CORES)

cache_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cache_model
	$(PIN_RUN) -mdc -mpf -n 2 -- $(TEST_APPS_DIR)/cache_model/cache_test

# below here is the splash benchmarks
fmm_test: sim 
	# note, the 5th line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TEST_APPS_DIR)/fmm
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=11 -- $(TEST_APPS_DIR)/fmm/FMM < $(TEST_APPS_DIR)/fmm/inputs/input.256

barnes_test: sim 
	# note, the last line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TEST_APPS_DIR)/barnes
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=5 -- $(TEST_APPS_DIR)/barnes/BARNES < $(TEST_APPS_DIR)/barnes/input


radiosity_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/radiosity
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=10 -- $(TEST_APPS_DIR)/radiosity/RADIOSITY -p 8 -batch -room

ocean_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ocean_contig
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_APPS_DIR)/ocean_contig/OCEAN -p8

raytrace_test: sim 
	#FIXME has some build issues
	$(MAKE) -C $(TEST_APPS_DIR)/raytrace
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_APPS_DIR)/raytrace/RAYTRACE -p8 $(TEST_APPS_DIR)/raytrace/inputs/teapot.env

volrend_test: sim 
	#FIXME this one runs out of memory
	$(MAKE) -C $(TEST_APPS_DIR)/volrend
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=45 -- $(TEST_APPS_DIR)/volrend/VOLREND 8 $(TEST_APPS_DIR)/volrend/inputs/head-scaleddown2

watern_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/water-nsquared
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_APPS_DIR)/water-nsquared/WATER-NSQUARED < $(TEST_APPS_DIR)/water-nsquared/input

waters_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/water-spatial
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_APPS_DIR)/water-spatial/WATER-SPATIAL < $(TEST_APPS_DIR)/water-spatial/input

