#!/usr/bin/env python

import sys
import os
import re
import subprocess
import time
import signal

# spawn_job:
#  start up a command across multiple machines
#  returns an object that can be passed to poll_job()
def spawn_job(machine_list, config_filename, command):

    working_dir = os.getcwd()
    procs = {}

    # spawn
    for i in range(0,len(machine_list)):
  
        if (machine_list[i] != "localhost") and (machine_list[i] != r'127.0.0.1'):
            if (command != ""):
                exec_command = command
            else:
                exec_command = "./tcp_ip -np " + str(len(machine_list)) + \
                    " -id " + str(i) + \
                    " -bp 2000 " + \
                    "< " + config_filename
            
            exec_command = "ssh -x " + machine_list[i] + \
                           " \"cd " + working_dir + "; " + \
                           exec_command + "\""
   
        print "[spawn.py] Starting process: " + str(i) + " : " + exec_command
        procs[i] = subprocess.Popen(exec_command, shell=True)

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
            else:
                os.kill(procs[i].pid, signal.SIGKILL)

    print "[spawn.py] Exited with return code: %d" % returnCode

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
def load_process_list_from_file(filename):
    process_list = []
    cfg = open(config_filename,"r").readlines()
    for line in cfg:
        hostname = re.search("([A-z0-9]+).*", line).group(1)
        process_list.append(hostname)

    return process_list

# main -- if this is used as standalone script
if __name__=="__main__":
    num_procs = int(sys.argv[1])
    command = " ".join(sys.argv[2:])
    
    config_filename = "process_list.dat"
    process_list = load_process_list_from_file(config_filename)

    j = spawn_job(process_list[0:num_procs], config_filename, command)

    sys.exit(wait_job(j))
