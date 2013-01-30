#!/usr/bin/env python

import re
import sys
import numpy
from optparse import OptionParser

global output_file_contents
global num_cores

def rowSearch(heading, key):
   global output_file_contents
   global num_cores

   key += "(.*)"
   heading_found = False
   
   for line in output_file_contents:
      if heading_found:
         match_key = re.search(key, line)
         if match_key:
            counts = line.split('|')
            # print len(counts)
            event_counts = counts[1:num_cores+1]
            for i in range(0, num_cores):
               if (len(event_counts[i].split()) == 0):
                  event_counts[i] = "0.0"
               #elif (event_counts[i].split()[0] == "nan"):
               #   event_counts[i] = "0.0"
            # print event_counts
            # print "%d" % len(event_counts)
            return map(lambda x: float(x), event_counts)
      else:
         if (re.search(heading, line)):
            heading_found = True

   print "ERROR: Could not find key [%s,%s]" % (heading, key)
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
parser.add_option("--input-file", dest="input_file", help="Graphite Input File")
parser.add_option("--stats-file", dest="stats_file", help="Statistics File")
parser.add_option("--num-cores", dest="num_cores", type="int", help="Number of Cores")
(options,args) = parser.parse_args()

# Read Results Files
try:
   output_file_contents = open(options.input_file, 'r').readlines()
except IOError:
   print "ERROR: Could not open file (%s)" % (options.input_file)
   sys.exit(3)

print "Parsing simulation output file: %s" % (options.input_file)
num_cores = options.num_cores

# Total Instructions
target_instructions = sum(rowSearch("Core Summary", "Total Instructions"))

# Completion Time - In nanoseconds
target_time = rowSearch("Core Summary", "Completion Time \(in ns\)")[0]

# Host Time
host_time = getTime("shutdown time")
host_initialization_time = getTime("start time")
host_working_time = getTime("shutdown time") - getTime("start time")
host_shutdown_time = getTime("shutdown time") - getTime("stop time")

# Write event counters to a file
stats_file = open(options.stats_file, 'w')
stats_file.write("Target-Instructions = %f\n" % (target_instructions))
stats_file.write("Target-Time = %f\n" % (target_time))
stats_file.write("Host-Time = %f\n" % (host_time))
stats_file.write("Host-Initialization-Time = %f\n" % (host_initialization_time))
stats_file.write("Host-Working-Time = %f\n" % (host_working_time))
stats_file.write("Host-Shutdown-Time = %f\n" % (host_shutdown_time))
stats_file.close()

print "Written stats file: %s" % (options.stats_file)
