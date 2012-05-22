#!/usr/bin/env python

import os
import sys

from file_reader import *

def execute(cmd):
   print cmd
   if (os.system(cmd) != 0):
      sys.exit(-1)
   
#benchmark_list = ['radix']
#benchmark_list = ['radix','fft','lu_contiguous','lu_non_contiguous','cholesky','ocean_contiguous','ocean_non_contiguous','barnes','fmm','water-nsquared','water-spatial','raytrace','radiosity','volrend']
benchmark_list = ['blackscholes','swaptions','canneal','fluidanimate','streamcluster','dynamic_graph']   

max_local_utilization = 31

L1_I_capacity_file = open("results/capacity/L1_I_wasted_capacity.out","w")
L1_D_capacity_file = open("results/capacity/L1_D_wasted_capacity.out","w")
L2_capacity_file = open("results/capacity/L2_wasted_capacity.out","w")

L1_I_capacity_file.write(" , ")
L1_D_capacity_file.write(" , ")
L2_capacity_file.write(" , ")
for utilization in range(0, max_local_utilization):
   L1_I_capacity_file.write("%i, " % utilization)
   L1_D_capacity_file.write("%i, " % utilization)
   L2_capacity_file.write("%i, " % utilization)
L1_I_capacity_file.write("\n")
L1_D_capacity_file.write("\n")
L2_capacity_file.write("\n")

for benchmark in benchmark_list:
   cmd = "./graphite_counters.py --input-file results/capacity/%s/sim.out --stats-file results/capacity/%s/stats.out" % (benchmark, benchmark)
   execute(cmd)
   
   # Read the statistics file
   stats_file = FileReader("results/capacity/%s/stats.out" % (benchmark))

   L1_I_capacity_file.write("%s, " % (benchmark))
   L1_D_capacity_file.write("%s, " % (benchmark))
   L2_capacity_file.write("%s, " % (benchmark))

   for utilization in range(0, max_local_utilization):
      # Eviction
      wasted_L1_I_capacity_eviction = stats_file.readFloat("wasted_capacity/utilization/%i/L1_I_eviction" % (utilization))
      wasted_L1_D_capacity_eviction = stats_file.readFloat("wasted_capacity/utilization/%i/L1_D_eviction" % (utilization))
      wasted_L2_capacity_eviction = stats_file.readFloat("wasted_capacity/utilization/%i/L2_eviction" % (utilization))
      # Invalidation
      wasted_L1_I_capacity_invalidation = stats_file.readFloat("wasted_capacity/utilization/%i/L1_I_invalidation" % (utilization))
      wasted_L1_D_capacity_invalidation = stats_file.readFloat("wasted_capacity/utilization/%i/L1_D_invalidation" % (utilization))
      wasted_L2_capacity_invalidation = stats_file.readFloat("wasted_capacity/utilization/%i/L2_invalidation" % (utilization))

      L1_I_capacity_file.write("%f, " % (wasted_L1_I_capacity_eviction + wasted_L1_I_capacity_invalidation))
      L1_D_capacity_file.write("%f, " % (wasted_L1_D_capacity_eviction + wasted_L1_D_capacity_invalidation))
      L2_capacity_file.write("%f, " % (wasted_L2_capacity_eviction + wasted_L2_capacity_invalidation))

   L1_I_capacity_file.write("\n")
   L1_D_capacity_file.write("\n")
   L2_capacity_file.write("\n")

L1_I_capacity_file.close()
L1_D_capacity_file.close()
L2_capacity_file.close()
