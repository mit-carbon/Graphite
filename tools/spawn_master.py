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

# spawn_job:
#  start up a command across multiple machines
#  returns an object that can be passed to poll_job()
def spawn_job(machine_list, command, working_dir = os.getcwd()):

    procs = {}
    # spawn
    for i in range(0, len(machine_list)):
        if (machine_list[i] == "localhost") or (machine_list[i] == r'127.0.0.1'):
            exec_command = "cd %s; %s" % (working_dir, command)
            
            print "[spawn_master.py] Starting process: %d: %s" % (i, exec_command)
            procs[i] = spawn.spawn_job(i, exec_command)
        else:
            spawn_slave_command = "%s/tools/spawn_slave.py %d %s" % (working_dir, i, command)
            exec_command = "ssh -x %s %s" % (machine_list[i], spawn_slave_command)
   
            print "[spawn_master.py] Starting process: %d: %s" % (i, exec_command)
            procs[i] = subprocess.Popen(exec_command, shell=True, preexec_fn=os.setsid)

    return procs

# poll_job:
#  check if a job has finished
#  returns the returnCode, or None
def poll_job(procs):
    
    # check status
    returnCode = None

    for i in range(0,len(procs)):
        returnCode = procs[i].poll()
        if returnCode != None:
            break

    # process still running
    if returnCode == None:
        return None

    # process terminated, so wait or kill remaining
    for i in range(0,len(procs)):
        returnCode2 = procs[i].poll()
        if returnCode2 == None:
            if returnCode == 0:
                returnCode = procs[i].wait()
                print "[spawn_master.py] Process: %d exited with return code: %d" % (i, returnCode)
            else:
                print "[spawn_master.py] Killing process: %d" % (i)
                os.killpg(procs[i].pid, signal.SIGKILL)
        else:
            print "[spawn_master.py] Process: %d exited with return code: %d" % (i, returnCode2)

    print "[spawn_master.py] Exited with return code: %d\n" % returnCode
    return returnCode

# wait_job:
#  wait on a job to finish
def wait_job(procs):

    while True:
        ret = poll_job(procs)
        if ret != None:
            return ret
        time.sleep(0.5)

# helper functions

# read process list from a config file
def load_process_list_from_file(config_filename):
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

# get sim root from environment variable, or use pwd
def get_sim_root():

    sim_root = os.environ.get('GRAPHITE_HOME')

    if sim_root == None:
        pwd = os.environ.get('PWD')
        print "\n[spawn_master.py] 'WARNING: GRAPHITE_HOME' undefined. Setting 'GRAPHITE_HOME' to '" + pwd
        return pwd

    return sim_root

# main -- if this is used as standalone script
if __name__=="__main__":
    num_procs = int(sys.argv[1])
    config_filename = sys.argv[2]
    command = " ".join(sys.argv[3:])

    process_list = load_process_list_from_file(config_filename)

    j = spawn_job(process_list[0:num_procs],
                  command,
                  get_sim_root())

    sys.exit(wait_job(j))
