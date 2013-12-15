#!/usr/bin/env python

import sys
import os
import re
import numpy
from config import *

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
         return "%dh %dm %ds" % (hours, minutes, seconds)


def geomean(num_list):
   if (len(num_list) == 0):
      return 0.0
   return numpy.product(num_list) ** (1.0 / len(num_list))

# Print a summary of the regression tests
summary_file = open("./tools/regress/summary.log", 'w')
host_time_list = {}
MIPS_list = {}
for num_machines in num_machines_list:
   host_time_list[num_machines] = []
   MIPS_list[num_machines] = []

summary_file.write("_" * 130)
summary_file.write("\n\n")
summary_file.write("%s | %s | %s |  %s |  %s | %s | %s | %s |\n" % \
                   ('Benchmark'.center(21), '# Machines'.center(12), 'Status'.center(8),
                    'Host'.center(13), 'Simulation'.center(13),
                    'Target'.center(10), 'Target'.center(10),
                    'Target'.center(18)) )
summary_file.write("%s | %s | %s |  %s |  %s | %s | %s | %s |\n" % \
                   (''.center(21), ''.center(12), ''.center(8),
                    'Time'.center(13), 'Performance'.center(13),
                    'Time'.center(10), 'Energy'.center(10),
                    'Performance/Watt'.center(18)) )
summary_file.write("%s | %s | %s |  %s |  %s | %s | %s | %s |\n" % \
                   (''.center(21), ''.center(12), ''.center(8),
                    ''.center(13), '(in MIPS)'.center(13),
                    '(in ms)'.center(10), '(in mJ)'.center(10),
                    '(in MIPS/W)'.center(18)) )
summary_file.write("_" * 130)
summary_file.write("\n\n")

for benchmark in benchmark_list:
   for num_machines in num_machines_list:
      
      # Don't aggregate results for benchmarks that work only in lite mode on multiple machines
      if (benchmark in lite_mode_list) and (num_machines > 1):
         continue

      cmd = "python -u ./tools/parse_output.py --results-dir %s/%s--procs-%i --num-cores 64" \
            % (results_dir, benchmark, num_machines)
      print cmd
      ret = os.system(cmd)

      if (ret == 0):
         eventCounterInfo = open("%s/%s--procs-%i/stats.out" % (results_dir, benchmark, num_machines), 'r').readlines()

         target_instructions = parseEventCounters("Target-Instructions", eventCounterInfo)
         
         host_time = parseEventCounters("Host-Time", eventCounterInfo)                                # In micro-seconds
         host_working_time = parseEventCounters("Host-Working-Time", eventCounterInfo)                # In micro-seconds
         MIPS = target_instructions / host_working_time
         formatted_MIPS = "%.2f" % (MIPS)
         
         target_time = 1.0e-6 * parseEventCounters("Target-Time", eventCounterInfo)     # In milli-seconds
         target_energy = 1000.0 * parseEventCounters("Target-Energy", eventCounterInfo) # In milli-joules
         target_performance_per_watt = 1.0e-3 * target_instructions / target_energy     # In MIPS/W

         summary_file.write("%s | %s | %s |  %s |  %s | %s | %s | %s |\n" % \
                            (benchmark.ljust(21), str(num_machines).center(12), 'PASS'.center(8),
                             formatTime(host_time).ljust(13), formatted_MIPS.ljust(13),
                             ("%.2f" % (target_time)).ljust(10), ("%.2f" % (target_energy)).ljust(10),
                             ("%.2f" % (target_performance_per_watt)).ljust(18)))
        
         host_time_list[num_machines].append(host_time)
         MIPS_list[num_machines].append(MIPS)
      else:
         summary_file.write("%s | %s | %s |  %s |  %s | %s | %s | %s |\n" % \
                            (benchmark.ljust(21), str(num_machines).center(12), 'FAIL'.center(8),
                             ''.ljust(13), ''.ljust(13),
                             ''.ljust(10), ''.ljust(10), ''.ljust(18)))

summary_file.write("_" * 130)
summary_file.write("\n\n")

for num_machines in num_machines_list:
   summary_file.write("%s Machines  : Host Time - %s (total), Simulation Performance - %.2f MIPS (geomean)\n" % \
         (num_machines, formatTime(sum(host_time_list[num_machines])), geomean(MIPS_list[num_machines])))
summary_file.close()
