#!/usr/bin/env python

import sys
import os

sys.path.append("./tools/")

from schedule import *
from config import *

# Compile all benchmarks first
for benchmark in benchmark_list:
   if benchmark in splash2_list:
      os.system("make %s_bench_test BUILD_MODE=build" % (benchmark))
   else: # In PARSEC list
      if (not os.path.exists("tests/parsec/parsec-3.0")):
         print "[regress] Creating PARSEC applications directory."
         os.system("make setup_parsec")
      os.system("make %s_parsec BUILD_MODE=build" % (benchmark))

# Generate jobs
jobs = []

for benchmark in benchmark_list:

   # Generate command
   if benchmark in splash2_list:
      command = "make %s_bench_test" % (benchmark)
   else: # In parsec_list
      command = "make %s_parsec" % (benchmark)

   # Get APP_FLAGS
   app_flags = None
   if benchmark in app_flags_map:
      app_flags = app_flags_map[benchmark]
      
   # Work in lite/full mode?
   if benchmark in lite_mode_list:
      mode = "lite"
   else: # Works in full & lite modes
      mode = "full"

   for num_machines in num_machines_list:
      # Don't schedule the benchmarks that work only in lite mode on multiple machines
      if (benchmark in lite_mode_list) and (num_machines > 1):
         continue

      # Generate SIM_FLAGS
      sim_flags = "--general/total_cores=64 " + \
                  "--general/enable_shared_mem=true " + \
                  "--general/mode=%s " % (mode) + \
                  "--general/enable_power_modeling=true " + \
                  "--general/trigger_models_within_application=true "

      # Generate sub_dir where results are going to be placed
      sub_dir = "%s--procs-%i" % (benchmark, num_machines)

      jobs.append(MakeJob(num_machines, command, config_filename, results_dir, sub_dir, sim_flags, app_flags, "pin"))

try:
   # Remove the results directory
   shutil.rmtree(results_dir)
   # Create results directory
   os.makedirs(results_dir)
except OSError:
   pass
   
# Go!
schedule(machines, jobs)
