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

import spawn
from termcolors import *

# spawn_job:
#  start up a command across multiple machines
#  returns an object that can be passed to poll_job()
def spawn_job(machine_list, command, working_dir, graphite_home):
    
    graphite_procs = {}

    # spawn
    for i in range(0, len(machine_list)):
        if (machine_list[i] == "localhost") or (machine_list[i] == r'127.0.0.1'):
            exec_command = "%s" % (command)
            
            print "%s Starting process: %d: %s" % (pmaster(), i, exec_command)
            graphite_procs[i] = spawn.spawn_job(i, exec_command, graphite_home)
        else:
            command = command.replace("\"", "\\\"")
            spawn_slave_command = "python -u %s/tools/spawn_slave.py %s %d \\\"%s\\\"" % (graphite_home, working_dir, i, command)
            exec_command = "ssh -x %s \"%s\"" % (machine_list[i], spawn_slave_command)
   
            print "%s Starting process: %d: %s" % (pmaster(), i, exec_command)
            graphite_procs[i] = subprocess.Popen(exec_command, shell=True, preexec_fn=os.setsid)

    return graphite_procs

# poll_job:
#  check if a job has finished
#  returns the returnCode, or None
def poll_job(graphite_procs):
    
    # check status
    returnCode = None

    for i in range(0,len(graphite_procs)):
        returnCode = graphite_procs[i].poll()
        if returnCode != None:
            break

    # process still running
    if returnCode == None:
        return None

    # process terminated, so wait or kill remaining
    for i in range(0,len(graphite_procs)):
        returnCode2 = graphite_procs[i].poll()
        if returnCode2 == None:
            if returnCode == 0:
                returnCode = graphite_procs[i].wait()
                print "%s Process: %d exited with ReturnCode: %d" % (pmaster(), i, returnCode)
            else:
                print "%s Killing process: %d" % (pmaster(), i)
                os.killpg(graphite_procs[i].pid, signal.SIGKILL)
        else:
            try:
                os.killpg(graphite_procs[i].pid, signal.SIGKILL)
            except OSError:
                pass
            print "%s Process: %d exited with ReturnCode: %d" % (pmaster(), i, returnCode2)

    print "%s %s\n" % (pmaster(), pReturnCode(returnCode))
    return returnCode

# wait_job:
#  wait on a job to finish
def wait_job(graphite_procs):
    
     while True:
         ret = poll_job(graphite_procs)
         if ret != None:
             return ret
         time.sleep(0.5)

# kill_job:
#  kill all graphite processes
def kill_job(graphite_procs):
   
    # Kill graphite processes
    for i in range(0,len(graphite_procs)):
        returnCode = graphite_procs[i].poll()
        if returnCode == None:
            print "%s Killing process: %d" % (pmaster(), i)
            os.killpg(graphite_procs[i].pid, signal.SIGKILL)
    
    returnCode = signal.SIGINT
    print "%s %s\n" % (pmaster(), pReturnCode(returnCode))
    return returnCode

# Helper functions

# Read output_dir from the command string
def get_output_dir(command):
    output_dir_match = re.match(r'.*--general/output_dir\s*=\s*([^\s]+)', command)
    if output_dir_match:
        return output_dir_match.group(1)
    
    print "*ERROR* Could not read output dir"
    sys.exit(-1)

# Read config filename from the command string
def get_config_filename(command):
    config_filename_match = re.match(r'.*\s+-c\s+([^\s]+\.cfg)\s+', command)
    if config_filename_match:
        return config_filename_match.group(1)
    
    print "*ERROR* Could not read config filename"
    sys.exit(-1)

# Read number of processes (from command string. If not found, from the config file)
def get_num_processes(command):
    proc_match = re.match(r'.*--general/num_processes\s*=\s*([0-9]+)', command)
    if proc_match:
        return int(proc_match.group(1))
    
    config_filename = get_config_filename(command)
    config = open(config_filename, 'r').readlines()
    
    found_general = False
    for line in config:
        if found_general == True:
            proc_match = re.match(r'\s*num_processes\s*=\s*([0-9]+)', line)
            if proc_match:
                return int(proc_match.group(1))
        else: 
            if re.match(r'\s*\[general\]', line):
                found_general = True

    print "*ERROR* Could not read number of processes to start the simulation"
    sys.exit(-1)

# Read process list (from command string. If not found, from the config file)
def get_process_list(command, num_processes):
    process_list = []

    curr_process_num = 0
    while True:
        hostname_match = re.match(r'.*--process_map/process%d\s*=\s*([A-Za-z0-9.]+)' % (curr_process_num), command)
        if hostname_match:
            process_list.append(hostname_match.group(1))
            curr_process_num = curr_process_num + 1
            if (curr_process_num == num_processes):
                return process_list
        else:
            break

    if (curr_process_num > 0):
        print "*ERROR* Found location of at least one process but not all processes from the command string"
        sys.exit(-1)

    
    config_filename = get_config_filename(command)
    config = open(config_filename).readlines()
    
    found_process_map = False
    for line in config:
        if found_process_map == True:
            hostname_match = re.match(r'\s*process%d\s*=\s*\"([A-Za-z0-9.]+)\"' % (curr_process_num), line)
            if hostname_match:
                process_list.append(hostname_match.group(1))
                curr_process_num = curr_process_num + 1
                if curr_process_num == num_processes:
                    return process_list
        else: 
            if re.match(r'\s*\[process_map\]', line):
                found_process_map = True

    print "*ERROR* Could not read process list from config file"
    sys.exit(-1)

# Create output dir
def create_output_dir(command, graphite_home):
    output_dir = get_output_dir(command)
    config_filename = get_config_filename(command)
    os.system("mkdir -p %s" % (output_dir))
    os.system("echo \"%s\" > %s/command" % (command, output_dir))
    os.system("cp %s %s/carbon_sim.cfg" % (config_filename, output_dir))
    os.system("rm -f %s/results/latest" % (graphite_home))
    os.system("ln -s %s %s/results/latest" % (output_dir, graphite_home))
    
# pmaster:
#  print spawn_master.py preamble
def pmaster():
    return colorstr('[spawn_master.py]', 'BOLD')

# main -- if this is used as standalone script
if __name__=="__main__":
   
    command = " ".join(sys.argv[1:])
    num_processes = get_num_processes(command)
    process_list = get_process_list(command, num_processes)
    working_dir = os.getcwd()

    # Get graphite home
    graphite_home = spawn.get_graphite_home()
    
    # Create output dir
    create_output_dir(command, graphite_home)

    # Spawn job
    graphite_procs = spawn_job(process_list,
                               command,
                               working_dir,
                               graphite_home)

    try:
        sys.exit(wait_job(graphite_procs))

    except KeyboardInterrupt:
        msg = colorstr('Keyboard interrupt. Killing simulation', 'RED')
        print "%s %s" % (pmaster(), msg)
        sys.exit(kill_job(graphite_procs))
