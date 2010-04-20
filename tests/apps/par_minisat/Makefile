TARGET = par_minisat

SOURCES = simple.cc solver.cc
#SOURCES = main.cc solver.cc
CORES = 5 # the parent thread counts, so make sure to include it here. If your app needs 4 worker processes and the main thread doesn't count as a worker process, add an extra core here.

# LARGER INPUT FILE:
INPUT_FILE ?= tests/apps/par_minisat/inputs/hacked/unif2p-p0.7-v3500-c9345-S1286605994-07-SAT--7000.cnf  # change this on command line to use different parameter

# SMALLER INPUT FILE:
#INPUT_FILE ?= tests/apps/par_minisat/inputs/hacked/unif2p-p0.7-v3500-c9345-S1286605994-07-SAT--200.cnf # change this on command line to use different parameter


APP_FLAGS ?= $(INPUT_FILE)

include ../../Makefile.tests
