#!/usr/bin/env python

import sys
import os
sys.path.append(os.getcwd() + '/tools')
import tests_infrastructure

plot_config_filename = "tests.cfg"
plot_data_directory = "/afs/csail.mit.edu/u/h/harshad/research/simulator/distrib/carbon_sim/results/2009_07_15__01_28_50/"
runs = [0]
expecting_file_name = 0
expecting_dir_name = 0
assert(len(sys.argv) <= 4)
for argument in sys.argv:
   if expecting_file_name == 1:
      plot_config_filename = argument
      expecting_file_name = 0
   elif expecting_dir_name == 1:
      plot_data_directory = argument
      expecting_dir_name = 0
   elif argument == '-f':
      expecting_file_name = 1
   elif argument == '-d':
      expecting_dir_name = 1
   elif argument == '-h':
      tests_infrastructure.print_plot_help_message()

curr_num_procs = tests_infrastructure.parse_config_file_params(plot_config_filename)
tests_infrastructure.generate_pintool_args(tests_infrastructure.parse_fixed_param_list(), curr_num_procs)

tests_infrastructure.generate_plot_directory_list(plot_data_directory, runs)
tests_infrastructure.generate_plots()

print tests_infrastructure.aggregate_stats

for quantity in tests_infrastructure.plot_quantities_list:
   str = quantity + ' '
   for directory in tests_infrastructure.plot_directory_list:
      str = str + tests_infrastructure.aggregate_stats[quantity][directory] + ' '
   print str
