cannon_unit_test: simlib
	$(MAKE) -C tests/unit/cannon
	./tests/unit/cannon/cannon -m 4 -s 4

spawn_unit_test: simlib
	$(MAKE) -C tests/unit/spawn
	./tests/unit/spawn/spawn

simple_dist_unit_test: simlib
	$(MAKE) -C tests/unit/simple_dist
	mpirun -np 2 ./tests/unit/simple_dist/simple_dist
