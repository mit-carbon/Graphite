#!/bin/bash

# Edit gcc.bldconf to set correct paths for CC_HOME and BINUTIL_HOME
echo "[BLDCONF] Editing gcc.bldconf with correct paths for CC_HOME and BINUTIL_HOME"
patch -p1 < setup_parsec_2.1/gcc_bldconf.patch

# Populate the PARSEC directories with the build configuration needed
# for building the tools (yasm,cmake,libtool). The configuration is
# derived from gcc because we do not need Graphite-specific flags
# for building tools
echo "[BLDCONF] Populating PARSEC directories with graphite.bldconf extending gcc.bldconf"
./bin/bldconfadd -n graphite -s gcc -f

# Build the tools (yasm,cmake,libtool). These tools are built in
# advance so that they need not be built during the time of
# building & running the benchmarks.
echo "[BUILD] Building tools (yasm, cmake, libtool)"
./bin/parsecmgmt -a build -p tools -c graphite

# Populate the PARSEC directories with the build configuration needed
# for Graphite. The configuration is derived from gcc-hooks because
# we would like to have the option to simulate just the parallel
# portion of the benchmark. This command just creates the relevant
# files, the actual compiler and linker flags are put in later
echo "[BLDCONF] Populating PARSEC directories with graphite.bldconf extending gcc-hooks.bldconf"
./bin/bldconfadd -n graphite -s gcc-hooks -f

# Apply changes to graphite.bldconf for the Graphite-specific
# compiler and linker flags
echo "[BLDCONF] Applying patches (CFLAGS, CXXFLAGS, LDFLAGS, LIBS) to graphite.bldconf"
patch -p1 < setup_parsec_2.1/graphite_bldconf.patch

# Apply changes for calling into Graphite when the hooks APIs'
# are called, __parsec_roi_begin() and __parsec_roi_end()
echo "[HOOKS] Applying graphite patches for hooks API"
patch -p1 < setup_parsec_2.1/hooks.patch

# Apply changes for spawning only as many threads as requested.
# By default, if N threads are requested, N+1 threads are spawned
# The changes are applied only to 5 benchmarks:
# blackscholes, swaptions, canneal, fluidanimate, streamcluster
echo "[THREADS] Applying thread spawn patches for blackscholes, swaptions, canneal, fluidanimate, streamcluster"
patch -p1 < setup_parsec_2.1/threads.patch

# Apply changes for opening file in read-only mode in freqmine.
# These changes are not required for running with Graphite but
# are put in for our regression tests to run. They do not affect
# the simulator performance/accuracy in any way.
echo "[FREQMINE] Applying open file mode patch for freqmine"
patch -p1 < setup_parsec_2.1/freqmine.patch

# Untar the simmedium inputs of PARSEC into benchmark-specific
# run/ directories. While running the benchmarks with the simulator,
# untaring inputs is unnecessary.
echo "[INPUTS] Untar'ing and setting up inputs in the run/ directory"
./setup_parsec_2.1/inputs.sh
