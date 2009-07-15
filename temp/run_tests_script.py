#!/usr/bin/env python

# Actual program starts here

import sys
import os
import tests_framework

sim_root = "./"
experiment_directory = "./"

tests_config_filename = "tests.cfg"
is_dryrun = 0
expecting_file_name = 0
assert(len(sys.argv) <= 4)
for argument in sys.argv:
   if expecting_file_name == 1:
      tests_config_filename = argument
      expecting_file_name = 0
   elif argument == '-dryrun':
      is_dryrun = 1
   elif argument == '-f':
      expecting_file_name = 1
   elif argument == '-h':
      tests_framework.print_help_message()

curr_num_procs = tests_framework.parse_config_file_params(tests_config_filename)
tests_framework.generate_simulation_args(tests_framework.parse_fixed_param_list(), curr_num_procs)

if is_dryrun == 0:
   from time import strftime
   experiment_directory = sim_root + "results/" + strftime("%Y_%m_%d__%H_%M_%S") + "/"
   mkdir_experiment_dir_command = "mkdir " + experiment_directory
   os.system(mkdir_experiment_dir_command)

   # Move config file
   cp_config_file_command = "cp " + sim_root + tests_config_filename + " " + experiment_directory
   os.system(cp_config_file_command)

tests_framework.run_simulation(experiment_directory, is_dryrun)
