SIM_ROOT ?= $(CURDIR)/../../
TEST_UNIT_DIR = $(SIM_ROOT)/tests/unit/

TEST_UNIT_LIST = cannon_unit_test spawn_unit_test simple_dist_unit_test barrier_unit_test mutex_unit_test
TEST_UNIT_DIRS = $(patsubst %_unit_test,%,$(TEST_UNIT_LIST))

clean:
	for t in $(TEST_UNIT_DIRS) ; do make -C $(TEST_UNIT_DIR)/$$t clean ; done

cannon_unit_test: $(SIM_ROOT)/lib/libcarbon_sim.a
	$(MAKE) -C tests/unit/cannon

spawn_unit_test: $(SIM_ROOT)/lib/libcarbon_sim.a
	$(MAKE) -C tests/unit/spawn

simple_dist_unit_test: $(SIM_ROOT)/lib/libcarbon_sim.a
	$(MAKE) -C tests/unit/simple_dist

barrier_unit_test: $(SIM_ROOT)/lib/libcarbon_sim.a
	$(MAKE) -C tests/unit/barrier

mutex_unit_test: $(SIM_ROOT)/lib/libcarbon_sim.a
	$(MAKE) -C tests/unit/mutex
