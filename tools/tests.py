#!/usr/bin/env python

import sys
import os

sys.path.append("./tools/")

from schedule import *

# job info
machines = [
    "cagnode1",
    "cagnode1",
    "cagnode2",
    "cagnode2",
    "cagnode3",
    "cagnode3",
    "cagnode4",
    "cagnode4",
    "cagnode8",
    "cagnode8",
    "cagnode9",
    "cagnode9"
    ]

results_dir = "./results/64-core"
cfg_file = "carbon_sim_64.cfg"

benchmarks = ["fft", "radix", "barnes", "ocean_contiguous", "fmm", "lu_contiguous"]
commands = ["./tests/benchmarks/fft/fft -p64 -m16",
            "./tests/benchmarks/radix/radix -p64",
            "./tests/benchmarks/barnes/barnes \< ./tests/benchmarks/barnes/input",
            "./tests/benchmarks/ocean_contiguous/ocean_contiguous -p64",
            "./tests/benchmarks/fmm/fmm \< ./tests/benchmarks/fmm/inputs/input.65536",
            "./tests/benchmarks/lu_contiguous/lu_contiguous -p64"]
protocols = ["ackwise","full_map","limited_no_broadcast"]
networks = ["emesh","atac"]
hardware_sharer_count = 4

jobs = []

jobs.append(LocalJob(12, "echo Starting"))

for benchmark, command in zip(benchmarks, commands):
   for protocol in protocols:
      for network in networks:
         sim_flags = "-c %s/%s --general/total_cores=64 --general/enable_shared_mem=true --network/memory_model_1=finite_buffer_%s --perf_model/dram_directory/directory_type=%s --perf_model/dram_directory/max_hw_sharers=%i" % (results_dir, cfg_file, network, protocol, hardware_sharer_count)
         sub_dir = "%s-%s-%s" % (benchmark, protocol, network)
         jobs.append(MakeJob(1, command, results_dir, sub_dir, sim_flags, "pin"))

jobs.append(LocalJob(12, "echo Finished"))

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
