#!/usr/bin/env python

import sys
import os
sys.path.append(os.getcwd() + '/tools')
import tests_infrastructure
import subprocess

# Actual program starts here
sim_root = "./"

tests_config_filename = "tests.cfg"
is_dryrun = 0
num_runs = 1
expecting_file_name = 0
expecting_num_runs = 0

assert(len(sys.argv) <= 4)
for argument in sys.argv:
   if expecting_file_name == 1:
      tests_config_filename = argument
      expecting_file_name = 0
   elif expecting_num_runs == 1:
      num_runs = int(argument)
      expecting_num_runs = 0
   elif argument == '-dryrun':
      is_dryrun = 1
   elif argument == '-f':
      expecting_file_name = 1
   elif argument == '-h':
      print_help_message()
   elif argument == '-n':
      expecting_num_runs = 1

if is_dryrun == 0:
   from time import strftime
   experiment_directory = sim_root + "results/" + strftime("%Y_%m_%d__%H_%M_%S") + "/"
   mkdir_experiment_dir_command = "mkdir " + experiment_directory
   os.system(mkdir_experiment_dir_command)

   # Move config file
   cp_config_file_command = "cp " + sim_root + tests_config_filename + " " + experiment_directory
   os.system(cp_config_file_command)

curr_num_procs = tests_infrastructure.parse_config_file_params(tests_config_filename)
tests_infrastructure.generate_pintool_args(tests_infrastructure.parse_fixed_param_list(), curr_num_procs)

for i in range(0, num_runs):
   tests_infrastructure.run_simulation(is_dryrun, i, sim_root, experiment_directory)
