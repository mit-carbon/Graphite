#!/usr/bin/env python

import os
import sys

from file_reader import *

def execute(cmd):
   print cmd
   if (os.system(cmd) != 0):
      sys.exit(-1)
   
benchmark_list = ['radix','fft','lu_contiguous','lu_non_contiguous','ocean_contiguous','ocean_non_contiguous','barnes']
#benchmark_list = ['radix']
#benchmark_list = ['radix','fft','lu_contiguous','lu_non_contiguous','cholesky','ocean_contiguous','ocean_non_contiguous','barnes','fmm','water-nsquared','water-spatial','raytrace','radiosity','volrend']
#benchmark_list = ['blackscholes','swaptions','canneal','fluidanimate','streamcluster','dynamic_graph']   
   
max_private_copy_threshold = 16

max_sharer_count_file = open("results/max_sharer_count.out","w")
avg_sharer_count_file = open("results/average_sharer_count.out","w")

max_sharer_count_file.write(" , ")
avg_sharer_count_file.write(" , ")
for private_copy_threshold in range(1, max_private_copy_threshold+1):
   max_sharer_count_file.write("%i, " % (private_copy_threshold))
   avg_sharer_count_file.write("%i, " % (private_copy_threshold))
max_sharer_count_file.write("\n")
avg_sharer_count_file.write("\n")

for benchmark in benchmark_list:
   cmd = "./graphite_counters.py --input-file results/sharing_1024/%s/sim.out --stats-file results/sharing_1024/%s/stats.out" % (benchmark, benchmark)
   execute(cmd)
   
   # Read the statistics file
   stats_file = FileReader("results/sharing_1024/%s/stats.out" % (benchmark))

   max_sharer_count_file.write("%s, " % (benchmark))
   avg_sharer_count_file.write("%s, " % (benchmark))

   for private_copy_threshold in range(1, max_private_copy_threshold+1):
      max_sharer_count = stats_file.readFloat("private_copy_threshold/%i/max_sharer_count" % (private_copy_threshold))
      avg_sharer_count = stats_file.readFloat("private_copy_threshold/%i/average_sharer_count" % (private_copy_threshold))

      max_sharer_count_file.write("%f, " % (max_sharer_count))
      avg_sharer_count_file.write("%f, " % (avg_sharer_count))

   max_sharer_count_file.write("\n")
   avg_sharer_count_file.write("\n")

max_sharer_count_file.close()
avg_sharer_count_file.close()
