#!/usr/bin/env python

import sys
import os

sys.path.append("./tools/")

from schedule import *

# job info
machines = [
    "cagnode1",
    "cagnode2",
    "cagnode3",
    "cagnode4"
    ]

results_dir = "./results/cache-line-replication"
cfg_file = "carbon_sim_64.cfg"

benchmarks = ["fft", "radix", "barnes", "ocean_contiguous", "fmm", "lu_contiguous","lu_non_contiguous","ocean_non_contiguous","cholesky","water-nsquared","water-spatial","raytrace","volrend"]
commands = ["./tests/benchmarks/fft/fft -p64 -m16",
            "./tests/benchmarks/radix/radix -p64",
            "./tests/benchmarks/barnes/barnes < ./tests/benchmarks/barnes/input",
            "./tests/benchmarks/ocean_contiguous/ocean_contiguous -p64",
            "./tests/benchmarks/fmm/fmm < ./tests/benchmarks/fmm/inputs/input.16384",
            "./tests/benchmarks/lu_contiguous/lu_contiguous -p64",
            "./tests/benchmarks/lu_non_contiguous/lu_non_contiguous -p64 -n512",
            "./tests/benchmarks/ocean_non_contiguous/ocean_non_contiguous -p64",
            "./tests/benchmarks/cholesky/cholesky -p64 ./tests/benchmarks/cholesky/inputs/tk15.O",
            "./tests/benchmarks/water-nsquared/water-nsquared < ./tests/benchmarks/water-nsquared/input",
            "./tests/benchmarks/water-spatial/water-spatial < ./tests/benchmarks/water-spatial/input",
            "./tests/benchmarks/raytrace/raytrace -p64 -m64 ./tests/benchmarks/raytrace/inputs/car.env",
            "./tests/benchmarks/volrend/volrend 64 ./tests/benchmarks/volrend/inputs/head"]

jobs = []

jobs.append(LocalJob(4, "echo Starting"))

for benchmark, command in zip(benchmarks, commands):
   sim_flags = "-c %s/%s --general/total_cores=64 --general/enable_shared_mem=true --network/memory_model_1=emesh_hop_by_hop" % (results_dir, cfg_file)
   print sim_flags
   sub_dir = "%s" % (benchmark)
   print sub_dir
   jobs.append(MakeJob(1, command, results_dir, sub_dir, sim_flags, "pin"))

jobs.append(LocalJob(4, "echo Finished"))

# jobs = [
#     LocalJob(1, "echo Starting..."),
#     MakeJob(1, command, results_dir, "1", sim_flags),
#     MakeJob(1, command, results_dir, "2", sim_flags),
#     MakeJob(1, command, results_dir, "3", sim_flags),
#     MakeJob(1, command, results_dir, "4", sim_flags),
#     LocalJob(1, "echo Finished."),
#     ]
# 
# uncomment this line to kill all the fmms
#jobs = [ SpawnJob(12, "killall -9 fmm") ]

# init
try:
    os.makedirs(results_dir)
except OSError:
    pass

shutil.copy(cfg_file, results_dir)

# go!
schedule(machines, jobs)
