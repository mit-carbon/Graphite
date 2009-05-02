#!/usr/bin/python

import sys
import subprocess

num_procs = int(sys.argv[1])

path = ""
for p in sys.argv[2:len(sys.argv)]:
    path += p + " "

#print "Executing %d copies of program: %s" % (num_procs, path)

procs = {}

for i in range(num_procs):
    env = {}
    env["CARBON_PROCESS_INDEX"] = str(i)
    env["LD_LIBRARY_PATH"] = "/afs/csail/group/carbon/tools/boost_1_38_0/stage/lib"
    
    procs[i] = subprocess.Popen(path, shell=True, env=env)

for i in range(num_procs):
    procs[i].wait()
