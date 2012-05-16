#!/usr/bin/env python

import re
import sys
from numpy import *
from optparse import OptionParser
import numpy

global graphiteResults
global numCores

def rowSearch(heading, key):
   global graphiteResults
   global numCores
   key += "(.*)"
   
   headingFound = False
   
   for line in graphiteResults:
      if headingFound:
         matchKey = re.search(key, line)
         if matchKey:
            counts = line.split('|')
            assert(len(counts) == 1028)
            eventCounts = counts[1:numCores+1]
            for i in range(0, numCores):
               if (len(eventCounts[i].split()) == 0):
                  eventCounts[i] = "0.0"
               elif (eventCounts[i].split()[0] == "nan"):
                  eventCounts[i] = "0.0"
            #print eventCounts
            #print "%d" % size(eventCounts)
            return array(map(lambda x: float(x), eventCounts))
      else:
         if (re.search(heading, line)):
            headingFound = True

   print "\n\n****  ERROR  ***** Could not Find [%s,%s]\n\n" % (heading, key)
   sys.exit(-1)

def rowSearchL3(heading, subHeading, key):
   global graphiteResults
   global numCores
   key += "(.*)"
   
   headingFound = False
   subHeadingFound = False

   for line in graphiteResults:
      if headingFound:
         if subHeadingFound:
            matchKey = re.search(key, line)
            if matchKey:
               counts = line.split('|')
               assert(len(counts) == 1028)
               eventCounts = counts[1:numCores+1]
               for i in range(0, numCores):
                  if (len(eventCounts[i].split()) == 0):
                     eventCounts[i] = "0.0"
                  elif (eventCounts[i].split()[0] == "nan"):
                     eventCounts[i] = "0.0"
               #print eventCounts
               #print "%d" % size(eventCounts)
               return array(map(lambda x: float(x), eventCounts))
         else:
            if (re.search(subHeading, line)):
               subHeadingFound = True
      else:
         if (re.search(heading, line)):
            headingFound = True

   print "\n\n****  ERROR  ***** Could not Find [%s,%s,%s]\n\n" % (heading, subHeading, key)
   sys.exit(-1)

def rowSearchL4(heading, subHeading, subSubHeading, key):
   global graphiteResults
   global numCores
   key += "(.*)"
   
   headingFound = False
   subHeadingFound = False
   subSubHeadingFound = False

   for line in graphiteResults:
      if headingFound:
         if subHeadingFound:
            if subSubHeadingFound:
               matchKey = re.search(key, line)
               if matchKey:
                  counts = line.split('|')
                  assert(len(counts) == 1028)
                  eventCounts = counts[1:numCores+1]
                  for i in range(0, numCores):
                     if (len(eventCounts[i].split()) == 0):
                        eventCounts[i] = "0.0"
                     elif (eventCounts[i].split()[0] == "nan"):
                        eventCounts[i] = "0.0"
                  #print eventCounts
                  #print "%d" % size(eventCounts)
                  return array(map(lambda x: float(x), eventCounts))
            else:
               if (re.search(subSubHeading, line)):
                  subSubHeadingFound = True
         else:
            if (re.search(subHeading, line)):
               subHeadingFound = True
      else:
         if (re.search(heading, line)):
            headingFound = True

   print "\n\n****  ERROR  ***** Could not Find [%s,%s,%s,%s]\n\n" % (heading, subHeading, subSubHeading, key)
   sys.exit(-1)

def rowPeek(heading, subHeading):
   global graphiteResults
   subHeading += "(.*)"
   headingFound = False
   for line in graphiteResults:
      if headingFound:
         matchSubHeading = re.search(subHeading, line)
         if matchSubHeading:
            return True
      else:
         if (re.search(heading, line)):
            headingFound = True
   return False
# ------------------ Main Program ----------------------

parser = OptionParser()
parser.add_option("--input-file", dest="input_file", help="Graphite Input File")
parser.add_option("--stats-file", dest="stats_file", help="Statistics File")
(options,args) = parser.parse_args()

# Read Results Files
graphiteResults = open(options.input_file, 'r').readlines()
numCores = 1024

completion_time = rowSearch("Core Model Summary", "Completion Time \(in ns\)")[0]

total_instructions = sum(rowSearch("Core Model Summary", "Total Instructions"))

total_L1_I_read_accesses = sum(rowSearch("Cache L1-I", "Cache Accesses"))
total_L1_I_read_misses = sum(rowSearch("Cache L1-I", "Cache Misses"))

total_L1_D_read_accesses = sum(rowSearch("Cache L1-D", "Read Accesses"))
total_L1_D_read_misses = sum(rowSearch("Cache L1-D",  "Read Misses"))
total_L1_D_write_accesses = sum(rowSearch("Cache L1-D", "Write Accesses"))
total_L1_D_write_misses = sum(rowSearch("Cache L1-D",  "Write Misses"))

total_L2_read_accesses = sum(rowSearch("Cache L2", "Read Accesses"))
total_L2_read_misses = sum(rowSearch("Cache L2", "Read Misses"))
total_L2_write_accesses = sum(rowSearch("Cache L2", "Write Accesses"))
total_L2_write_misses = sum(rowSearch("Cache L2", "Write Misses"))

max_local_utilization = 15
max_private_copy_threshold = max_local_utilization + 1

# Look at the utilization due to invalidations now
total_invalidations_by_utilization = []
L1_I_read_utilization_invalidation = []
L1_D_read_utilization_invalidation = []
L1_D_write_utilization_invalidation = []
L2_read_utilization_invalidation = []
L2_write_utilization_invalidation = []
shared_state_count_invalidation = []
modified_state_count_invalidation = []
for i in range(0, max_local_utilization + 1):
   total_invalidations_by_utilization.append(sum(rowSearchL3("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i))))
   L1_I_read_utilization_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "L1-I read")))
   L1_D_read_utilization_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "L1-D read")))
   L1_D_write_utilization_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "L1-D write")))
   L2_read_utilization_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "L2 read")))
   L2_write_utilization_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "L2 write")))
   shared_state_count_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "Shared State")))
   modified_state_count_invalidation.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Invalidation\)", "Utilization-%i" % (i), "Modified State")))

# Look at the utilization due to invalidations now
total_evictions_by_utilization = []
L1_I_read_utilization_eviction = []
L1_D_read_utilization_eviction = []
L1_D_write_utilization_eviction = []
L2_read_utilization_eviction = []
L2_write_utilization_eviction = []
shared_state_count_eviction = []
modified_state_count_eviction = []
for i in range(0, max_local_utilization + 1):
   total_evictions_by_utilization.append(sum(rowSearchL3("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i))))
   L1_I_read_utilization_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "L1-I read")))
   L1_D_read_utilization_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "L1-D read")))
   L1_D_write_utilization_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "L1-D write")))
   L2_read_utilization_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "L2 read")))
   L2_write_utilization_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "L2 write")))
   shared_state_count_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "Shared State")))
   modified_state_count_eviction.append(sum(rowSearchL4("Cache Cntlr", "Utilization Summary \(Eviction\)", "Utilization-%i" % (i), "Modified State")))

## Directory side
total_shreq_modified_state = sum(rowSearch("Dram Directory Cntlr", "Shared Request - MODIFIED State"))
total_shreq_shared_state = sum(rowSearch("Dram Directory Cntlr", "Shared Request - SHARED State"))
total_shreq_uncached_state = sum(rowSearch("Dram Directory Cntlr", "Shared Request - UNCACHED State"))
total_shreq_dirty_evictions = sum(rowSearch("Cache Cntlr", "Shared Request - Dirty Evictions"))
total_shreq_clean_evictions = sum(rowSearch("Cache Cntlr", "Shared Request - Clean Evictions"))

total_exreq_modified_state = sum(rowSearch("Dram Directory Cntlr", "Exclusive Request - MODIFIED State"))
total_exreq_shared_state = sum(rowSearch("Dram Directory Cntlr", "Exclusive Request - SHARED State"))
total_exreq_upgrade_reply = sum(rowSearch("Dram Directory Cntlr", "Exclusive Request - Upgrade Reply"))
total_exreq_uncached_state = sum(rowSearch("Dram Directory Cntlr", "Exclusive Request - UNCACHED State"))
total_exreq_dirty_evictions = sum(rowSearch("Cache Cntlr", "Exclusive Request - Dirty Evictions"))
total_exreq_clean_evictions = sum(rowSearch("Cache Cntlr", "Exclusive Request - Clean Evictions"))

total_invalidations_at_directory_row = rowSearch("Dram Directory Cntlr", "Total Invalidation Requests - Unicast Mode")
average_sharers_invalidated_row = rowSearch("Dram Directory Cntlr", "Average Sharers Invalidated - Unicast Mode")
total_exreq_shared_state_invalidations = sum(total_invalidations_at_directory_row * average_sharers_invalidated_row)

total_shreq_modified_state_with_wbrep = []
total_shreq_shared_state_with_wbrep = []
total_exreq_modified_state_with_flushrep = []
total_exreq_shared_state_with_invrep = []
total_exreq_shared_state_with_flushrep = []
for i in range(0, max_local_utilization + 1):
   # ShReq
   total_shreq_modified_state_with_wbrep.append(sum(rowSearchL3("Dram Directory Cntlr", "Utilization Summary \(Shared Request - MODIFIED State - WB Rep\)", "Utilization-%i" % (i))))
   total_shreq_shared_state_with_wbrep.append(sum(rowSearchL3("Dram Directory Cntlr", "Utilization Summary \(Shared Request - SHARED State - WB Rep\)", "Utilization-%i" % (i))))
   # ExReq
   total_exreq_modified_state_with_flushrep.append(sum(rowSearchL3("Dram Directory Cntlr", "Utilization Summary \(Exclusive Request - MODIFIED State - FLUSH Rep\)", "Utilization-%i" % (i))))
   total_exreq_shared_state_with_invrep.append(sum(rowSearchL3("Dram Directory Cntlr", "Utilization Summary \(Exclusive Request - SHARED State - INV Rep\)", "Utilization-%i" % (i))))
   total_exreq_shared_state_with_flushrep.append(sum(rowSearchL3("Dram Directory Cntlr", "Utilization Summary \(Exclusive Request - SHARED State - FLUSH Rep\)", "Utilization-%i" % (i))))

#### Capacity-related

wasted_L1_I_capacity_on_eviction_by_utilization = [0] * (max_local_utilization+1)
wasted_L1_D_capacity_on_eviction_by_utilization = [0] * (max_local_utilization+1)
wasted_L2_capacity_on_eviction_by_utilization = [0] * (max_local_utilization+1)

wasted_L1_I_capacity_on_invalidation_by_utilization = [0] * (max_local_utilization+1)
wasted_L1_D_capacity_on_invalidation_by_utilization = [0] * (max_local_utilization+1)
wasted_L2_capacity_on_invalidation_by_utilization = [0] * (max_local_utilization+1)

for i in range (0, max_local_utilization+1):
   # Compute wasted L1-I,L1-D,L2 capacity
   # Eviction
   wasted_L1_I_capacity_on_eviction_by_utilization[i] = 100.0 * numpy.average(rowSearchL4("Cache Cntlr", "Lifetime Summary \(Eviction\)", "Utilization-%i" % (i), "L1-I Lifetime")) / completion_time
   wasted_L1_D_capacity_on_eviction_by_utilization[i] = 100.0 * numpy.average(rowSearchL4("Cache Cntlr", "Lifetime Summary \(Eviction\)", "Utilization-%i" % (i), "L1-D Lifetime")) / completion_time
   wasted_L2_capacity_on_eviction_by_utilization[i] = 100.0 * numpy.average(rowSearchL4("Cache Cntlr", "Lifetime Summary \(Eviction\)", "Utilization-%i" % (i), "L2 Lifetime")) / completion_time
   
   # Invalidation
   wasted_L1_I_capacity_on_invalidation_by_utilization[i] = 100.0 * numpy.average(rowSearchL4("Cache Cntlr", "Lifetime Summary \(Invalidation\)", "Utilization-%i" % (i), "L1-I Lifetime")) / completion_time
   wasted_L1_D_capacity_on_invalidation_by_utilization[i] = 100.0 * numpy.average(rowSearchL4("Cache Cntlr", "Lifetime Summary \(Invalidation\)", "Utilization-%i" % (i), "L1-D Lifetime")) / completion_time
   wasted_L2_capacity_on_invalidation_by_utilization[i] = 100.0 * numpy.average(rowSearchL4("Cache Cntlr", "Lifetime Summary \(Invalidation\)", "Utilization-%i" % (i), "L2 Lifetime")) / completion_time

#### Sharer-related

## Max sharers
max_sharer_count_by_private_copy_threshold = [0] * (max_private_copy_threshold + 1)
for i in range(1, max_private_copy_threshold + 1):
   max_sharer_count_by_private_copy_threshold[i] = max(rowSearchL3("Dram Directory Cntlr", "Sharer Count by Private Copy Threshold", "Maximum-PCT-%i" % (i)))

## Average sharers
total_invalidations_row = rowSearch("Dram Directory Cntlr", "Total Invalidations")
total_invalidations = sum(total_invalidations_row)
average_sharer_count_by_private_copy_threshold = [0] * (max_private_copy_threshold + 1)
for i in range(1, max_private_copy_threshold + 1):
   average_sharer_count_row = rowSearchL3("Dram Directory Cntlr", "Sharer Count by Private Copy Threshold", "Average-PCT-%i" % (i))
   total_sharer_count = sum(average_sharer_count_row * total_invalidations_row)
   average_sharer_count_by_private_copy_threshold[i] = total_sharer_count / total_invalidations

# Write event counters to a file
stats_file = open(options.stats_file, 'w')
stats_file.write("[core]\n")
stats_file.write("instructions = %f\n" % (total_instructions))
stats_file.write("\n")

stats_file.write("[L1_I_cache]\n")
stats_file.write("read_accesses = %f\n" % (total_L1_I_read_accesses))
stats_file.write("read_misses = %f\n" % (total_L1_I_read_misses))
stats_file.write("\n")

stats_file.write("[L1_D_cache]\n")
stats_file.write("read_accesses = %f\n" % (total_L1_D_read_accesses))
stats_file.write("read_misses = %f\n" % (total_L1_D_read_misses))
stats_file.write("write_accesses = %f\n" % (total_L1_D_write_accesses))
stats_file.write("write_misses = %f\n" % (total_L1_D_write_misses))
stats_file.write("\n")

stats_file.write("[L2_cache]\n")
stats_file.write("read_accesses = %f\n" % (total_L2_read_accesses))
stats_file.write("read_misses = %f\n" % (total_L2_read_misses))
stats_file.write("write_accesses = %f\n" % (total_L2_write_accesses))
stats_file.write("write_misses = %f\n" % (total_L2_write_misses))
stats_file.write("\n")

stats_file.write("[utilization]\n")
stats_file.write("max_local = %i\n" % (max_local_utilization))
stats_file.write("\n")

for i in range (0, max_local_utilization + 1):
   stats_file.write("[invalidation/utilization/%i]\n" % (i))
   stats_file.write("L1_I_read = %f\n" % (L1_I_read_utilization_invalidation[i]))
   stats_file.write("L1_D_read = %f\n" % (L1_D_read_utilization_invalidation[i]))
   stats_file.write("L1_D_write = %f\n" % (L1_D_write_utilization_invalidation[i]))
   stats_file.write("L2_read = %f\n" % (L2_read_utilization_invalidation[i]))
   stats_file.write("L2_write = %f\n" % (L2_write_utilization_invalidation[i]))
   stats_file.write("shared_state = %f\n" % (shared_state_count_invalidation[i]))
   stats_file.write("modified_state = %f\n" % (modified_state_count_invalidation[i]))
   stats_file.write("\n")

for i in range (0, max_local_utilization + 1):
   stats_file.write("[eviction/utilization/%i]\n" % (i))
   stats_file.write("L1_I_read = %f\n" % (L1_I_read_utilization_eviction[i]))
   stats_file.write("L1_D_read = %f\n" % (L1_D_read_utilization_eviction[i]))
   stats_file.write("L1_D_write = %f\n" % (L1_D_write_utilization_eviction[i]))
   stats_file.write("L2_read = %f\n" % (L2_read_utilization_eviction[i]))
   stats_file.write("L2_write = %f\n" % (L2_write_utilization_eviction[i]))
   stats_file.write("shared_state = %f\n" % (shared_state_count_eviction[i]))
   stats_file.write("modified_state = %f\n" % (modified_state_count_eviction[i]))
   stats_file.write("\n")

# Directory Side
stats_file.write("[MOSI]\n")
stats_file.write("shreq_modified_state = %f\n" % (total_shreq_modified_state))
stats_file.write("shreq_shared_state = %f\n" % (total_shreq_shared_state))
stats_file.write("shreq_uncached_state = %f\n" % (total_shreq_uncached_state))
stats_file.write("shreq_dirty_eviction = %f\n" % (total_shreq_dirty_evictions))
stats_file.write("shreq_clean_eviction = %f\n" % (total_shreq_clean_evictions))

stats_file.write("exreq_modified_state = %f\n" % (total_exreq_modified_state))
stats_file.write("exreq_shared_state = %f\n" % (total_exreq_shared_state))
stats_file.write("exreq_upgrade_rep = %f\n" % (total_exreq_upgrade_reply))
stats_file.write("exreq_uncached_state = %f\n" % (total_exreq_uncached_state))
stats_file.write("exreq_dirty_eviction = %f\n" % (total_exreq_dirty_evictions))
stats_file.write("exreq_clean_eviction = %f\n" % (total_exreq_clean_evictions))
stats_file.write("exreq_shared_state_invalidations = %f\n" % (total_exreq_shared_state_invalidations))
stats_file.write("\n")

# ShReq
stats_file.write("[MOSI/shreq_modified_state/wb_rep]\n")
for i in range (0, max_local_utilization + 1):
   stats_file.write("utilization-%i = %f\n" % (i, total_shreq_modified_state_with_wbrep[i]))
stats_file.write("\n")

stats_file.write("[MOSI/shreq_shared_state/wb_rep]\n")
for i in range (0, max_local_utilization + 1):
   stats_file.write("utilization-%i = %f\n" % (i, total_shreq_shared_state_with_wbrep[i]))
stats_file.write("\n")

# ExReq
stats_file.write("[MOSI/exreq_modified_state/flush_rep]\n")
for i in range (0, max_local_utilization + 1):
   stats_file.write("utilization-%i = %f\n" % (i, total_exreq_modified_state_with_flushrep[i]))
stats_file.write("\n")

stats_file.write("[MOSI/exreq_shared_state/inv_rep]\n")
for i in range (0, max_local_utilization + 1):
   stats_file.write("utilization-%i = %f\n" % (i, total_exreq_shared_state_with_invrep[i]))
stats_file.write("\n")

stats_file.write("[MOSI/exreq_shared_state/flush_rep]\n")
for i in range (0, max_local_utilization + 1):
   stats_file.write("utilization-%i = %f\n" % (i, total_exreq_shared_state_with_flushrep[i]))
stats_file.write("\n")

##### Capacity-related
for i in range(0, max_local_utilization):
   stats_file.write("\n[wasted_capacity/utilization/%i]\n" % (i))
   # Eviction
   stats_file.write("L1_I_eviction = %f\n" % (wasted_L1_I_capacity_on_eviction_by_utilization[i]))
   stats_file.write("L1_D_eviction = %f\n" % (wasted_L1_D_capacity_on_eviction_by_utilization[i]))
   stats_file.write("L2_eviction = %f\n" % (wasted_L2_capacity_on_eviction_by_utilization[i]))
   # Invalidation
   stats_file.write("L1_I_invalidation = %f\n" % (wasted_L1_I_capacity_on_invalidation_by_utilization[i]))
   stats_file.write("L1_D_invalidation = %f\n" % (wasted_L1_D_capacity_on_invalidation_by_utilization[i]))
   stats_file.write("L2_invalidation = %f\n" % (wasted_L2_capacity_on_invalidation_by_utilization[i]))

##### Sharer-related
for i in range(1, max_private_copy_threshold + 1):
   stats_file.write("\n[private_copy_threshold/%i]\n" % (i))
   stats_file.write("max_sharer_count = %i\n" % (max_sharer_count_by_private_copy_threshold[i]))
   stats_file.write("average_sharer_count = %f\n" % (average_sharer_count_by_private_copy_threshold[i]))
   
stats_file.close()
