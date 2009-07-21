#!/usr/bin/env python

"""
This is the client process that distributes n simulators
"""

import sys
import os
import re
import subprocess
import time
import signal

procs = {}

def load_process_list_from_file(filename):
    process_list = []
    config = open(config_filename,"r").readlines()
    found_process_map = False
    for line in config:
        if found_process_map:
            if line == "\n":
                break;
            # extract the process from the line
            hostname = re.search("\"(.*)\"", line).group(1)
            process_list.append(hostname)

        if line == "[process_map]\n":
            found_process_map = True

    return process_list


# Look through the process list and ssh to these machines
# Run launch.py on these machines
sim_root = os.environ.get('GRAPHITE_ROOT')

if sim_root == None:
   pwd = os.environ.get('PWD')
   assert(pwd != None)
   print "\n\n'GRAPHITE_ROOT' undefined. \nSetting 'GRAPHITE_ROOT' to '" + pwd + "'\n\n"
   sim_root = pwd

num_procs = int(sys.argv[1])
config_filename = "carbon_sim.cfg"
command = ""

for i in range(2,len(sys.argv)):
    command += sys.argv[i] + " "

# Spawn all the processes on the machines as per the config file
process_list = load_process_list_from_file(config_filename)
for i in range(0,num_procs):
  
   exec_command = "./tools/launch.py " + str(i) + " 1 " + command
   if (process_list[i] != "localhost") and (process_list[i] != r'127.0.0.1'):
      exec_command = "ssh -x " + process_list[i] + " \"cd " + sim_root + "; " + exec_command + "\""
   
   print "[carbon_sim_runner.py] Starting process: " + str(i) + " : " + exec_command
   procs[i] = subprocess.Popen(exec_command, shell=True)

# Wait for all the spawned processes to exit
# Code copied from $(GRAPHITE_ROOT)/tools/launch.py
# I dont understand some of this code
active = True
returnCode = None

while active:
   time.sleep(0.1)
   for i in range(0,num_procs):
      returnCode = procs[i].poll()
      if returnCode != None:
         active = False
         break

# kill
for i in range(0,num_procs):
   returnCode2 = procs[i].poll()
   if returnCode2 == None:
      if returnCode == 0:
         returnCode = procs[i].wait()
      else:
         os.kill(procs[i].pid, signal.SIGKILL)

# exit
if returnCode != None:
    print "[carbon_sim_runner.py] Exited with return code: %d" % returnCode

sys.exit(returnCode)
