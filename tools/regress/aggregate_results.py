#!/usr/bin/env python

import sys
import os
import re
import numpy

def parseEventCounters(event, eventCounterInfo):
   for counterInfo in eventCounterInfo:
      match = re.search(event+"\s*=\s*([-e0-9.]+)", counterInfo)
      if match:
         return float(match.group(1))
   print "ERROR: Could not Find Event Counter(%s)" % (event)
   sys.exit(1)

def formatTime(time):
   seconds = int (time / 1.0e6)
   if (seconds < 60):
      return "%ds" % (seconds)
   else:
      minutes = seconds / 60
      seconds = seconds % 60
      if (minutes < 60):
         return "%dm %ds" % (minutes, seconds)
      else:
         hours = minutes / 60
         minutes = minutes % 60
         return "%dd %dm %ds" % (hours, minutes, seconds)


def geomean(num_list):
   return numpy.product(num_list) ** (1.0 / len(num_list))

results_dir = "./tools/regress/simulation_results"
benchmarks = ["fft", "radix", "lu_contiguous", "lu_non_contiguous", "cholesky", "barnes", "fmm", "ocean_contiguous", "ocean_non_contiguous", "water-nsquared", "water-spatial", "raytrace", "volrend", "radiosity"]
num_machines_list = [1,2]

# Print a summary of the regression tests
summary_file = open("./tools/regress/summary.log", 'w')
host_time_list = []
KIPS_list = []

summary_file.write("_" * 100)
summary_file.write("\n\n")
summary_file.write("%s | %s | %s |  %s |  %s |\n" % ('Benchmarks'.center(23), '# Machines'.center(12), 'Status'.center(8), 'Simulated Time'.center(16), 'Performance (in KIPS)'.center(25)))
summary_file.write("_" * 100)
summary_file.write("\n\n")
for benchmark in benchmarks:
   for num_machines in num_machines_list:
      cmd = "\n./tools/parse_output.py --input-file %s/%s/sim.out --stats-file %s/%s/stats.out --num-cores 64" \
            % (results_dir, benchmark, results_dir, benchmark)
      print cmd
      ret = os.system(cmd)

      if (ret == 0):
         eventCounterInfo = open("%s/%s/stats.out" % (results_dir, benchmark), 'r').readlines()

         total_target_instructions = parseEventCounters("Target-Instructions", eventCounterInfo)
         host_time = parseEventCounters("Host-Time", eventCounterInfo)                                # In micro-seconds
         host_working_time = parseEventCounters("Host-Working-Time", eventCounterInfo)                # In micro-seconds
         KIPS = total_target_instructions * 1000 / host_working_time
         formatted_KIPS = "%.2f" % (KIPS)
         summary_file.write("%s | %s | %s |  %s |  %s |\n" % (benchmark.ljust(23), str(num_machines).center(12), 'PASS'.center(8), formatTime(host_time).ljust(16), formatted_KIPS.ljust(25)))
         
         host_time_list.append(host_time)
         KIPS_list.append(KIPS)
      else:
         summary_file.write("%s | %s | %s |  %s |  %s |\n" % (benchmark.ljust(23), str(num_machines).center(12), 'FAIL'.center(8), ''.ljust(16), ''.ljust(25)))
summary_file.write("_" * 100)
summary_file.write("\n")

summary_file.write("\n%s (total), %.2f KIPS (geomean)\n" % (formatTime(sum(host_time_list)), geomean(KIPS_list)))
summary_file.close()
