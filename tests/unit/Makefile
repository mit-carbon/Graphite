SIM_ROOT ?= $(CURDIR)/../../
TEST_UNIT_DIR = $(SIM_ROOT)/tests/unit/

TEST_UNIT_LIST = spawn_unit_test spawn_join_unit_test dynamic_threads_unit_test \
	barrier_unit_test mutex_unit_test file_io_unit_test pthreads_unit_test

regress_unit: $(TEST_UNIT_LIST)

clean:
	for t in $(patsubst %_unit_test,%,$(TEST_UNIT_LIST)) ; do make -C $(TEST_UNIT_DIR)/$$t clean ; done

%_unit_test:
	$(MAKE) -C $(TEST_UNIT_DIR)/$(patsubst %_unit_test,%,$@)

