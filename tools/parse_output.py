#!/usr/bin/env python

import re
import sys
import numpy
from optparse import OptionParser

global output_file_contents
global num_cores

def searchKey(key, line):
   global num_cores
   key += "(.*)"
   match_key = re.search(key, line)
   if match_key:
      counts = line.split('|')
      event_counts = counts[1:num_cores+1]
      for i in range(0, num_cores):
         if (len(event_counts[i].split()) == 0):
            event_counts[i] = "0.0"
      return map(lambda x: float(x), event_counts)
   return None

def rowSearch1(heading, key):
   global output_file_contents
   heading_found = False
   for line in output_file_contents:
      if heading_found:
         value = searchKey(key, line)
         if value:
            return value
      else:
         heading_found = (re.search(heading, line) != None)

   print "ERROR: Could not find key [%s,%s]" % (heading, key)
   sys.exit(1)

def rowSearch2(heading, sub_heading, key):
   global output_file_contents
   heading_found = False
   sub_heading_found = False
   for line in output_file_contents:
      if heading_found:
         if sub_heading_found:
            value = searchKey(key, line)
            if value:
               return value
         else:
            sub_heading_found = (re.search(sub_heading, line) != None)
      else:
         heading_found = (re.search(heading, line) != None)

   print "ERROR: Could not find key [%s,%s,%s]" % (heading, sub_heading, key)
   sys.exit(1)

def getTime(key):
   global output_file_contents

   key += "\s+([0-9]+)\s*"
   for line in output_file_contents:
      match_key = re.search(key, line)
      if match_key:
         return float(match_key.group(1))
   print "ERROR: Could not find key [%s]" % (key)
   sys.exit(2)

parser = OptionParser()
parser.add_option("--results-dir", dest="results_dir", help="Graphite Results Directory")
parser.add_option("--num-cores", dest="num_cores", type="int", help="Number of Cores")
(options,args) = parser.parse_args()

# Read Results Files
try:
   output_file_contents = open("%s/sim.out" % (options.results_dir), 'r').readlines()
except IOError:
   print "ERROR: Could not open file (%s/sim.out)" % (options.results_dir)
   sys.exit(3)

print "Parsing simulation output file: %s/sim.out" % (options.results_dir)
num_cores = options.num_cores

# Total Instructions
target_instructions = sum(rowSearch1("Core Summary", "Total Instructions"))

# Completion Time - In nanoseconds
target_time = max(rowSearch1("Core Summary", "Completion Time \(in nanoseconds\)"))
# Energy - In joules
core_energy = sum(rowSearch2("Tile Energy Monitor Summary", "Core", "Total Energy \(in J\)"))
cache_hierarchy_energy = sum(rowSearch2("Tile Energy Monitor Summary", "Cache Hierarchy \(L1-I, L1-D, L2\)", "Total Energy \(in J\)"))
networks_energy = sum(rowSearch2("Tile Energy Monitor Summary", "Networks \(User, Memory\)", "Total Energy \(in J\)"))
target_energy = core_energy + cache_hierarchy_energy + networks_energy

# Host Time
host_time = getTime("Shutdown Time \(in microseconds\)")
host_initialization_time = getTime("Start Time \(in microseconds\)")
host_working_time = getTime("Stop Time \(in microseconds\)") - getTime("Start Time \(in microseconds\)")
host_shutdown_time = getTime("Shutdown Time \(in microseconds\)") - getTime("Stop Time \(in microseconds\)")

# Write event counters to a file
stats_file = open("%s/stats.out" % (options.results_dir), 'w')

stats_file.write("Target-Instructions = %f\n" % (target_instructions))
stats_file.write("Target-Time = %f\n" % (target_time))
stats_file.write("Target-Energy = %f\n" % (target_energy))
stats_file.write("Target-Core-Energy = %f\n" % (core_energy))
stats_file.write("Target-Cache-Hierarchy-Energy = %f\n" % (cache_hierarchy_energy))
stats_file.write("Target-Networks-Energy = %f\n" % (networks_energy))

stats_file.write("Host-Time = %f\n" % (host_time))
stats_file.write("Host-Initialization-Time = %f\n" % (host_initialization_time))
stats_file.write("Host-Working-Time = %f\n" % (host_working_time))
stats_file.write("Host-Shutdown-Time = %f\n" % (host_shutdown_time))
stats_file.close()

print "Written stats file: %s/stats.out" % (options.results_dir)
