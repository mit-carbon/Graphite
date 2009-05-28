SIM_ROOT ?= $(CURDIR)/../../
TEST_BENCH_DIR = $(SIM_ROOT)/tests/benchmarks/
CLEAN=$(findstring clean,$(MAKECMDGOALS))

ifeq ($(CLEAN),)
include $(SIM_ROOT)/common/Makefile.common
endif

TEST_BENCH_LIST = fmm barnes radiosity raytrace volrend ocean_contig water-nsquared water-spatial

#TEST_APP_DIRS = $(patsubst %_test,%,$(TEST_UNIT_LIST))
TEST_BENCH_DIRS = $(TEST_BENCH_LIST)

# Run options
PIN_HOME = /afs/csail/group/carbon/tools/pin/current
PIN_BIN=$(PIN_HOME)/ia32/bin/pinbin
PIN_TOOL=$(SIM_ROOT)/lib/pin_sim
PIN_RUN=$(PIN_BIN) -mt -t $(PIN_TOOL)

clean_sim:
	$(MAKE) -C $(SIM_ROOT) clean

regress: regress_quick clean_benchmarks build_benchmarks 1djacobi_test_quick 

clean:
	for t in $(TEST_BENCH_DIRS) ; do make -C $(TEST_BENCH_DIR)/$$t clean ; done

build_benchmarks: sim
	for t in $(TEST_BENCH_DIRS) ; do make -C $(TEST_BENCH_DIR)/$$t ; done

sim:
	$(MAKE) -C $(SIM_ROOT)/common
	$(MAKE) -C $(SIM_ROOT)/pin
	$(MAKE) -C $(SIM_ROOT) 

fmm_test: sim 
	# note, the 5th line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TEST_BENCH_DIR)/fmm
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=11 -- $(TEST_BENCH_DIR)/fmm/FMM < $(TEST_BENCH_DIR)/fmm/inputs/input.256

barnes_test: sim 
	# note, the last line in the input file must match the number of procs passed to pin
	$(MAKE) -C $(TEST_BENCH_DIR)/barnes
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=5 -- $(TEST_BENCH_DIR)/barnes/BARNES < $(TEST_BENCH_DIR)/barnes/input


radiosity_test: sim 
	$(MAKE) -C $(TEST_BENCH_DIR)/radiosity
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=10 -- $(TEST_BENCH_DIR)/radiosity/RADIOSITY -p 8 -batch -room

ocean_test: sim 
	$(MAKE) -C $(TEST_BENCH_DIR)/ocean_contig
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_BENCH_DIR)/ocean_contig/OCEAN -p8

raytrace_test: sim 
	#FIXME has some build issues
	$(MAKE) -C $(TEST_BENCH_DIR)/raytrace
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_BENCH_DIR)/raytrace/RAYTRACE -p8 $(TEST_BENCH_DIR)/raytrace/inputs/teapot.env

volrend_test: sim 
	#FIXME this one runs out of memory
	$(MAKE) -C $(TEST_BENCH_DIR)/volrend
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=45 -- $(TEST_BENCH_DIR)/volrend/VOLREND 8 $(TEST_BENCH_DIR)/volrend/inputs/head-scaleddown2

watern_test: sim 
	$(MAKE) -C $(TEST_BENCH_DIR)/water-nsquared
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_BENCH_DIR)/water-nsquared/WATER-NSQUARED < $(TEST_BENCH_DIR)/water-nsquared/input

waters_test: sim 
	$(MAKE) -C $(TEST_BENCH_DIR)/water-spatial
	$(PIN_RUN)  -c $(SIM_ROOT)/carbon_sim.cfg  --general/total_cores=9 -- $(TEST_BENCH_DIR)/water-spatial/WATER-SPATIAL < $(TEST_BENCH_DIR)/water-spatial/input

