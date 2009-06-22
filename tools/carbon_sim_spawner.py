#!/usr/bin/env python

"""
This is the client process that distributes n simulators
"""

import socket
import sys
import re
import subprocess

def spawn_sim(host, id, path):
    port = 1999
    size = 1024
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host,port))
    s.send('%s%s,%s' % ('s',id,path))
    return s

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

# for spawning locally
def spawn_proc(command):
    env = {}
    env["CARBON_PROCESS_INDEX"] = str(0)
    env["LD_LIBRARY_PATH"] = "/afs/csail/group/carbon/tools/boost_1_38_0/stage/lib"
    proc = subprocess.Popen(command, shell=True, env=env)
    proc.wait()

# actual program
config_filename = "carbon_sim.cfg"
simulator_count = int(sys.argv[1])
command = ""
for i in range(2,len(sys.argv)):
    command += sys.argv[i] + " "

process_list = load_process_list_from_file(config_filename)

# determine if we will spawn locally or if we will distribute
if simulator_count > 1:
    sim_list = []
    # spawn the simulators
    for i in range(simulator_count):
        sim_list.append(spawn_sim(process_list[i],i,command))

    # wait for them to finish
    for s in sim_list:
        data = s.recv(65525)
        s.close()
        print 'Received:', data

else:
    spawn_proc(command)



