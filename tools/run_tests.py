#!/usr/bin/env python

# Look at a configuration file that tells which all tests to run
# Right now, run those tests one by one

# Import other configuration parameters from carbon_sim.cfg that you want
# to change

import os
import re
import sys
import subprocess

# TODO
# Add support for handling comments

pintool_fixed_param_list = []
pintool_variable_param_list = []
sim_flags_list = []
num_procs_list = []
sim_core_index_list = []
app_list = []
test_args_list = []
user_thread_index_list = []

def parse_config_file_params(tests_config_filename):
   tests_config_app = []
   tests_config_pintool = []

   parsing_app_list = 0
   parsing_pintool_params = 0

   try:
      tests_config = open(tests_config_filename,"r").readlines()
   except IOError:
      print "Cannot Open File: %s" % tests_config_filename
      sys.exit(-1)

   for line in tests_config:
      if re.search(r'^\[apps\]', line):
         parsing_app_list = 1
      elif re.search(r'^\[/apps\]', line):
         parsing_app_list = 0
      elif re.search(r'^\[pintool\]', line):
         parsing_pintool_params = 1
      elif re.search(r'^\[/pintool\]', line):
         parsing_pintool_params = 0
      elif parsing_app_list == 1:
         tests_config_app.append(line)
      elif parsing_pintool_params == 1:
         tests_config_pintool.append(line)

   parse_app_list(tests_config_app)
   curr_num_procs = parse_pintool_params(tests_config_pintool)
   
   return curr_num_procs

def parse_app_list(tests_config_app):
   global app_list
   for line in tests_config_app:
      if (re.search(r'[^\s]', line)):
         test_match = re.search(r'^\s*(\w+)(.*)', line)
         assert test_match
         test_name = test_match.group(1)
         test_args = test_match.group(2)

         parse_test_args(test_args)

         test_name_match = re.search(r'(.+)_([a-z]+)', test_name)
         assert test_name_match
         test_target_name = test_name_match.group(1)

         if (test_name_match.group(2) == 'app'):
            act_test_name = "./tests/apps/" + test_target_name + "/" + test_target_name
         elif (test_name_match.group(2) == 'bench'):
            act_test_name = "./tests/benchmarks/" + test_target_name + "/" + test_target_name
         else:
            assert false

         for test_instance in test_args_list:
            app_list.append(act_test_name + test_instance)
   return

def parse_test_args(test_args):
   global test_args_list
   global user_thread_index_list

   var_arg_list = re.findall(r'\[[^\]]+\]', test_args)
   
   expanded_var_arg_list = []
   num_threads_pos = -1
   iter = 0
   for var_arg in var_arg_list:
      [is_num_threads, expanded_var_arg] = expand_to_list(var_arg)
      if is_num_threads == 1:
         num_threads_pos = iter
      iter = iter + 1
      expanded_var_arg_list.append(expanded_var_arg)

   splitted_test_args = re.findall(r'[^\[\]]+', test_args)
   
   # Remove all the odd indices in splitted_test_args
   fixed_test_arg_list = []
   iter = 0
   for arg in splitted_test_args:
      if (iter % 2) == 0:
         fixed_test_arg_list.append(arg)
      iter = iter + 1

   if (len(fixed_test_arg_list) != (len(expanded_var_arg_list)+1)):
      assert len(fixed_test_arg_list) == len(expanded_var_arg_list)
      fixed_test_arg_list.append(" ")
  
   test_args_list = []
   generate_test_args(fixed_test_arg_list, expanded_var_arg_list, num_threads_pos)
      
   return

def generate_test_args(fixed_test_arg_list, expanded_var_arg_list, num_threads_pos, curr_num_threads_index = -1, curr_test_arg_list = "", index = 0):
   global test_args_list
   global user_thread_index_list

   if index == len(expanded_var_arg_list):
      test_args_list.append(curr_test_arg_list + fixed_test_arg_list[index])
      user_thread_index_list.append(curr_num_threads_index)
      return

   # Initialize curr_num_threads_index
   if index == num_threads_pos:
      curr_num_threads_index = 0

   for arg in expanded_var_arg_list[index]:
      generate_test_args(fixed_test_arg_list, expanded_var_arg_list, 
            num_threads_pos, curr_num_threads_index,
            curr_test_arg_list + fixed_test_arg_list[index] + arg, index + 1)
      
      if index == num_threads_pos:
         curr_num_threads_index = curr_num_threads_index + 1
   
   return

def expand_to_list(var_arg):
   value_list = re.findall(r'[+*\w]+', var_arg)

   is_num_threads = 0
   if re.search(r'\$', var_arg):
      is_num_threads = 1
   
   if (re.search(r',', var_arg)):
      return [is_num_threads, value_list]

   assert(re.search(r':', var_arg))

   act_value_list = []
   
   assert(len(value_list) == 3)
   start_val = int(value_list[0])
   end_val = int(value_list[1])
   curr_val = start_val
            
   if (value_list[2][0] == '+'):
      increment = int(value_list[2][1:])
      while (curr_val <= end_val):
         act_value_list.append(str(curr_val))
         curr_val += increment
   elif (value_list[2][0] == '*'):
      multiplier = int(value_list[2][1:])
      while (curr_val <= end_val):
         act_value_list.append(str(curr_val))
         curr_val *= multiplier
   else:
      # Raise an exception
      assert(false)

   return [is_num_threads, act_value_list]


def parse_pintool_params(tests_config_pintool):

   global pintool_fixed_param_list
   global pintool_variable_param_list

   param_value_list_regex = re.compile(r'(\w+)\s*=\s*(\[.+\])')
   param_value_regex = re.compile(r'(\w+)\s*=\s*(\w+)')
   section_name_regex = re.compile(r'\[(.*)\]')

   section_name = ""
   curr_num_procs = '0'
   
   for line in tests_config_pintool:
    
      param_value_list_match = param_value_list_regex.search(line)
      if param_value_list_match:
         param = param_value_list_match.group(1)
         act_value_list = expand_to_list(param_value_list_match.group(2))[1]

         pintool_param = [section_name + "/" + param, act_value_list]
         pintool_variable_param_list.append(pintool_param)
         continue

      param_value_match = param_value_regex.search(line)
      if param_value_match:
         param = param_value_match.group(1)
         value = param_value_match.group(2)

         pintool_param = [section_name + "/" + param, value]
         if pintool_param[0] == "general/num_processes":
            curr_num_procs = pintool_param[1]
         # Construct the parameter to be passed to the pintool
         pintool_fixed_param_list.append(pintool_param)
         continue
 
      section_name_match = section_name_regex.search(line)
      if section_name_match:
         section_name = section_name_match.group(1)
   
   return curr_num_procs

def generate_simulation_args(arg_list, curr_num_procs, curr_core_index = -1, variable_param_num = 0):
   global pintool_variable_param_list
   global pintool_fixed_param_list
   global sim_flags_list
   global num_procs_list
   global sim_core_index_list

   # Recursive calls are made to this function

   if variable_param_num == len(pintool_variable_param_list):
      sim_flags_list.append(arg_list)
      num_procs_list.append(curr_num_procs)
      sim_core_index_list.append(curr_core_index)
      return
   
   pintool_param = pintool_variable_param_list[variable_param_num]
   param_name = pintool_param[0]
   value_list = pintool_param[1]
      
   # Initialize 'curr_core_index'
   if (param_name == 'general/total_cores'):
      curr_core_index = 0

   for value in value_list:
      if (param_name == 'general/num_processes'):
         curr_num_procs = value

      new_arg_list = arg_list + "--" + param_name + "=" + value + " "
      
      generate_simulation_args(new_arg_list, curr_num_procs, curr_core_index, variable_param_num+1)
      
      if (param_name == 'general/total_cores'):
         curr_core_index = curr_core_index + 1

   return

def parse_fixed_param_list():
   global pintool_fixed_param_list
   fixed_arg_list = ""
   for pintool_param in pintool_fixed_param_list:
      fixed_arg_list += "--" + pintool_param[0] + "=" + pintool_param[1] + " "
   return fixed_arg_list

def run_simulation(is_dryrun):
   global sim_flags_list
   global num_procs_list
   global sim_core_index_list
   global app_list
   global user_thread_index_list

   global sim_root
   global experiment_directory

   pin_bin = "/afs/csail/group/carbon/tools/pin/current/ia32/bin/pinbin"
   pin_tool = sim_root + "lib/pin_sim"
   pin_run = pin_bin + " -mt -t " + pin_tool

   i = 0
   while i < len(sim_flags_list):
      j = 0
      while j < len(app_list):
         if (user_thread_index_list[j] == -1) or (sim_core_index_list[i] == -1) or (sim_core_index_list[i] == user_thread_index_list[j]):
            command = sim_root + "tools/carbon_sim_spawner.py " + num_procs_list[i] + " " + pin_run + " " + sim_flags_list[i] + " -- " + app_list[j]
            print command
            if is_dryrun == 0:
               proc = subprocess.Popen(command, shell=True)
               proc.wait()
               # Copy the results into a per-experiment per-run directory
               run_directory = experiment_directory + "ARGS_" + remove_unwanted_symbols(sim_flags_list[i]) + remove_unwanted_symbols(app_list[j]) + "/"
            
               mkdir_command = "mkdir " + run_directory
               os.system(mkdir_command)
            
               mv_command = "mv " + sim_root + "sim.out " + run_directory;
               os.system (mv_command)

               make_exec_file_command = "echo \"" + command + "\" > " + run_directory + "exec_command"
               os.system (make_exec_file_command)
         j = j+1
      i = i+1

   return

def remove_unwanted_symbols(in_string):
   
   i = 0
   out_string = ''
   while i < len(in_string):
      if in_string[i] == ' ':
         out_string += '___'
      elif in_string[i] == '-':
         pass
      elif in_string[i] == '/':
         out_string += '__'
      elif re.search(r'\w', in_string[i]):
         out_string += in_string[i]
      else:
         out_string += '_'
      i += 1

   return out_string


# Actual program starts here
sim_root = "./"

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

curr_num_procs = parse_config_file_params(tests_config_filename)
generate_simulation_args(parse_fixed_param_list(), curr_num_procs)

if is_dryrun == 0:
   from time import strftime
   experiment_directory = sim_root + "results/" + strftime("%Y_%m_%d__%H_%M_%S") + "/"
   mkdir_experiment_dir_command = "mkdir " + experiment_directory
   os.system(mkdir_experiment_dir_command)

   # Move config file
   cp_config_file_command = "cp " + sim_root + tests_config_filename + " " + experiment_directory
   os.system(cp_config_file_command)

run_simulation(is_dryrun)
