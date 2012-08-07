#!/usr/bin/env python

import sys
import os

sys.path.append("./tools/")

from schedule import *

# job info
# Do not use 'localhost' or '127.0.0.1', use the machine name
machines = [
    "cagnode5",
    "cagnode14",
    "cagnode15"
    ]

results_dir = "./results/test"
cfg_file = "carbon_sim.cfg"

benchmarks = ["fft", "radix", "lu_contiguous", "lu_non_contiguous", "cholesky", "barnes", "fmm", "ocean_contiguous", "ocean_non_contiguous", "water-nsquared", "water-spatial", "raytrace", "volrend"]
commands = ["./tests/benchmarks/fft/fft -p64 -m16",
            "./tests/benchmarks/radix/radix -p64",
            "./tests/benchmarks/lu_contiguous/lu_contiguous -p64",
            "./tests/benchmarks/lu_non_contiguous/lu_non_contiguous -p64 -n512",
            "./tests/benchmarks/cholesky/cholesky -p64 ./tests/benchmarks/cholesky/inputs/tk15.O",
            "./tests/benchmarks/barnes/barnes < ./tests/benchmarks/barnes/input",
            "./tests/benchmarks/fmm/fmm < ./tests/benchmarks/fmm/inputs/input.16384",
            "./tests/benchmarks/ocean_contiguous/ocean_contiguous -p64",
            "./tests/benchmarks/ocean_non_contiguous/ocean_non_contiguous -p64",
            "./tests/benchmarks/water-nsquared/water-nsquared < ./tests/benchmarks/water-nsquared/input",
            "./tests/benchmarks/water-spatial/water-spatial < ./tests/benchmarks/water-spatial/input",
            "./tests/benchmarks/raytrace/raytrace -p64 -m64 ./tests/benchmarks/raytrace/inputs/car.env",
            "./tests/benchmarks/volrend/volrend 64 ./tests/benchmarks/volrend/inputs/head"]

jobs = []

for benchmark, command in zip(benchmarks, commands):
   make_cmd = "make -C tests/benchmarks/%s clean && make %s_bench_test BUILD_MODE=build" % (benchmark, benchmark)
   os.system(make_cmd)
   
   sim_flags = "-c %s/%s --general/total_cores=64 --general/enable_shared_mem=true" % (results_dir, cfg_file)
   sub_dir = "%s" % (benchmark)
   print sim_flags
   print sub_dir
   jobs.append(MakeJob(1, command, results_dir, sub_dir, sim_flags, "pin"))

# jobs = [
#     MakeJob(1, command, results_dir, "1", sim_flags),
#     MakeJob(1, command, results_dir, "2", sim_flags),
#     MakeJob(1, command, results_dir, "3", sim_flags),
#     MakeJob(1, command, results_dir, "4", sim_flags),
#     ]

# init
try:
    os.makedirs(results_dir)
except OSError:
    pass

shutil.copy(cfg_file, results_dir)

# go!
schedule(machines, jobs)
