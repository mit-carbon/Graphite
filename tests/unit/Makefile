SIM_ROOT ?= $(CURDIR)/../../
TEST_UNIT_DIR = $(SIM_ROOT)/tests/unit/

TEST_UNIT_LIST = spawn_unit_test spawn_join_unit_test dynamic_threads_unit_test \
	barrier_unit_test mutex_unit_test file_io_unit_test pthreads_unit_test \
	read_write_unit_test
SHARED_MEM_UNIT_LIST = shared_mem_basic_unit_test shared_mem_test1_unit_test \
							  shared_mem_test2_unit_test shared_mem_test3_unit_test \
							  shared_mem_test4_unit_test shared_mem_test5_unit_test \
							  shared_mem_test6_unit_test

regress_unit: $(TEST_UNIT_LIST)
	
regress_shared_mem: $(SHARED_MEM_UNIT_LIST)

clean:
	for t in $(patsubst %_unit_test,%,$(TEST_UNIT_LIST)) ; do make -C $(TEST_UNIT_DIR)/$$t clean ; done

%_unit_test:
	$(MAKE) -C $(TEST_UNIT_DIR)/$(patsubst %_unit_test,%,$@)

