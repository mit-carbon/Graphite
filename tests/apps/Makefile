SIM_ROOT ?= $(CURDIR)/../../
TEST_APPS_DIR = $(SIM_ROOT)/tests/apps/
CLEAN=$(findstring clean,$(MAKECMDGOALS))

include $(SIM_ROOT)/tests/Makefile.pin

ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

TEST_APP_LIST = hello_world_test simple_test file_io_test ping_pong_test cannon_msg_test \
                cannon_test simple_dist_test cannon_msg_dist_test ring_msg_pass_test dynamic_threads_test \
		spawn_join_test

#TEST_APP_DIRS = $(patsubst %_test,%,$(TEST_APP_LIST))
TEST_APP_DIRS = hello_world simple file_io ping_pong cannon_msg \
                cannon simple_dist ring_msg_pass dynamic_threads \
		spawn_join

clean_sim:
	$(MAKE) -C $(SIM_ROOT) clean

regress: regress_quick 1djacobi_test_quick 

clean:
	for t in $(TEST_APP_DIRS) ; do make -C $(TEST_APPS_DIR)/$$t clean ; done

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

1djacobi_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/1d_jacobi
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/total_cores=$(CORES) -- $(TEST_APPS_DIR)/1d_jacobi/jacobi $(CORES) 64

1djacobi_test_quick: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/1d_jacobi
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/enable_shared_memory=true --general/total_cores=4 -- $(TEST_APPS_DIR)/1d_jacobi/jacobi 4 64

jacobi_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/shared_mem_jacobi
	$(PIN_RUN) -c $(SIM_ROOT)/carbon_sim.cfg --general/enable_shared_memory=true -msm -msys -mpf -n $(CORES) -dms 1000 -- $(TEST_APPS_DIR)/shared_mem_jacobi/jacobi -n $(CORES)

basic_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/pthreads_basic
	$(PIN_RUN) -mpf -mdc -msm -msys -n $(CORES) -dms 4 -- $(TEST_APPS_DIR)/pthreads_basic/basic -n $(CORES)

cache_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cache_model
	$(PIN_RUN) -mdc -mpf -n 2 -- $(TEST_APPS_DIR)/cache_model/cache_test
