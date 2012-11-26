#!/usr/bin/env python

import sys
import os

sys.path.append("./tools/")

from schedule import *

# job info
# Do not use 'localhost' or '127.0.0.1', use the machine name
machines = [
    "cagnode15",
    "cagnode1"
    ]

results_dir = "./tools/regress/simulation_results"

benchmarks = ["radix", "fft"]
commands = [
            "./tests/benchmarks/radix/radix -p64",
            "./tests/benchmarks/fft/fft -p64 -m16"
           ]

#benchmarks = ["fft", "radix", "lu_contiguous", "lu_non_contiguous", "cholesky", "barnes", "fmm", "ocean_contiguous", "ocean_non_contiguous", "water-nsquared", "water-spatial", "raytrace", "volrend", "radiosity"]
#commands = ["./tests/benchmarks/fft/fft -p64 -m16",
#            "./tests/benchmarks/radix/radix -p64",
#            "./tests/benchmarks/lu_contiguous/lu_contiguous -p64",
#            "./tests/benchmarks/lu_non_contiguous/lu_non_contiguous -p64 -n512",
#            "./tests/benchmarks/cholesky/cholesky -p64 ./tests/benchmarks/cholesky/inputs/tk15.O",
#            "./tests/benchmarks/barnes/barnes < ./tests/benchmarks/barnes/input",
#            "./tests/benchmarks/fmm/fmm < ./tests/benchmarks/fmm/inputs/input.16384",
#            "./tests/benchmarks/ocean_contiguous/ocean_contiguous -p64",
#            "./tests/benchmarks/ocean_non_contiguous/ocean_non_contiguous -p64",
#            "./tests/benchmarks/water-nsquared/water-nsquared < ./tests/benchmarks/water-nsquared/input",
#            "./tests/benchmarks/water-spatial/water-spatial < ./tests/benchmarks/water-spatial/input",
#            "./tests/benchmarks/raytrace/raytrace -p64 -m64 ./tests/benchmarks/raytrace/inputs/car.env",
#            "./tests/benchmarks/volrend/volrend 64 ./tests/benchmarks/volrend/inputs/head",
#            "./tests/benchmarks/radiosity/radiosity -p 64 -batch -room"]

jobs = []

for benchmark, command in zip(benchmarks, commands):
   make_cmd = "make -C tests/benchmarks/%s clean && make %s_bench_test BUILD_MODE=build" % (benchmark, benchmark)
   os.system(make_cmd)
 
# Generate jobs
for benchmark, command in zip(benchmarks, commands):
   sim_flags = "-c carbon_sim.cfg --general/total_cores=64 --general/enable_shared_mem=true --general/trigger_models_within_application=true"
   sub_dir = "%s" % (benchmark)
   jobs.append(MakeJob(1, command, results_dir, sub_dir, sim_flags, "pin"))

# Make results directory
try:
    os.makedirs(results_dir)
except OSError:
    pass

# Go!
#schedule(machines, jobs)
