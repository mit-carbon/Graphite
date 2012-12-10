#!/usr/bin/env python

import sys
import os

sys.path.append("./tools/")

from schedule import *
from config import *

jobs = []

for benchmark in benchmark_list:
   make_cmd = "make -C tests/benchmarks/%s clean && make %s_bench_test BUILD_MODE=build" % (benchmark, benchmark)
   os.system(make_cmd)
 
# Generate jobs
for benchmark, command in zip(benchmark_list, command_list):
   for num_machines in num_machines_list:
      sim_flags = "-c carbon_sim.cfg --general/total_cores=64 --general/enable_shared_mem=true --general/trigger_models_within_application=true"
      sub_dir = "%s-%i" % (benchmark, num_machines)
      jobs.append(MakeJob(num_machines, command, results_dir, sub_dir, sim_flags, "pin"))

# Remove the results directory
try:
   shutil.rmtree(results_dir)
except OSError:
   pass
   
# Create results directory
try:
   os.makedirs(results_dir)
except OSError:
   pass

# Go!
schedule(machines, jobs)
