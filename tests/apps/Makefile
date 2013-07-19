SIM_ROOT ?= $(CURDIR)/../../
TEST_APPS_DIR = $(SIM_ROOT)/tests/apps/

TEST_APP_LIST = empty_main_app_test hello_world_app_test simple_app_test \
	ping_pong_app_test cannon_app_test 1d_jacobi_app_test ring_msg_pass_app_test \
	2d_jacobi_shmem_app_test matrix_multiply_shmem_app_test \
	all_to_all_app_test
TEST_DIST_LIST = hello_world_app_test_dist simple_app_test_dist \
	ping_pong_app_test_dist cannon_app_test_dist 1d_jacobi_app_test_dist ring_msg_pass_app_test_dist \
	2d_jacobi_shmem_app_test_dist matrix_multiply_shmem_app_test_dist \
	all_to_all_app_test_dist

regress_apps: $(TEST_APP_LIST) $(TEST_DIST_LIST)

ifeq ($(MAKECMDGOALS),clean)
clean:
	for t in $(patsubst %_app_test,%,$(TEST_APP_LIST)) ; do make -C $(TEST_APPS_DIR)/$$t clean ; done
endif

%_app_test_dist:
	$(MAKE) -C $(TEST_APPS_DIR)/$(patsubst %_app_test_dist,%,$@) PROCS=2

%_app_test:
	$(MAKE) -C $(TEST_APPS_DIR)/$(patsubst %_app_test,%,$@)
