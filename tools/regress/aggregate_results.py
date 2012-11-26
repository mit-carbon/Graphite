#!/usr/bin/env python

import sys
import os
import re
import numpy
from defines import *

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

# Print a summary of the regression tests
summary_file = open("./tools/regress/summary.log", 'w')
host_time_list_1 = []
KIPS_list_1 = []
host_time_list_2 = []
KIPS_list_2 = []

summary_file.write("_" * 100)
summary_file.write("\n\n")
summary_file.write("%s | %s | %s |  %s |  %s |\n" % ('Benchmarks'.center(23), '# Machines'.center(12), 'Status'.center(8), 'Simulation Time'.center(16), 'Performance (in KIPS)'.center(25)))
summary_file.write("_" * 100)
summary_file.write("\n\n")
for num_machines in num_machines_list:
   for benchmark in benchmarks:
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
         
         if (num_machines == 1):
            host_time_list_1.append(host_time)
            KIPS_list_1.append(KIPS)
         else:
            host_time_list_2.append(host_time)
            KIPS_list_2.append(KIPS)
      else:
         summary_file.write("%s | %s | %s |  %s |  %s |\n" % (benchmark.ljust(23), str(num_machines).center(12), 'FAIL'.center(8), ''.ljust(16), ''.ljust(25)))
summary_file.write("_" * 100)
summary_file.write("\n\n")

summary_file.write("1 Machine  : Simulation Time - %s (total), Performance - %.2f KIPS (geomean)\n" % (formatTime(sum(host_time_list_1)), geomean(KIPS_list_1)))
summary_file.write("2 Machines : Simulation Time - %s (total), Performance - %.2f KIPS (geomean)\n" % (formatTime(sum(host_time_list_2)), geomean(KIPS_list_2)))
summary_file.close()
