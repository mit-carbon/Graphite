SIM_ROOT ?= $(CURDIR)/../../
TEST_APPS_DIR = $(SIM_ROOT)/tests/apps/
CLEAN=$(findstring clean,$(MAKECMDGOALS))

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

simple_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/simple

spawn_join_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/spawn_join

dynamic_threads_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/dynamic_threads

simple_dist_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/simple_dist

cannon_msg_dist_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cannon_msg

ring_msg_pass_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ring_msg_pass

file_io_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/file_io

ping_pong_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ping_pong

ping_pong_dist_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/ping_pong

cannon_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cannon

cannon_msg_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cannon_msg

capi_worker_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/capi_worker

1djacobi_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/1d_jacobi

1djacobi_test_quick: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/1d_jacobi

jacobi_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/shared_mem_jacobi

basic_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/pthreads_basic

cache_test: sim 
	$(MAKE) -C $(TEST_APPS_DIR)/cache_model
