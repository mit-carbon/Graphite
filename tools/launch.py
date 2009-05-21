#!/usr/bin/python

import sys
import subprocess

start_proc = int(sys.argv[1])
num_procs = int(sys.argv[2])

path = ""
for p in sys.argv[3:len(sys.argv)]:
    path += p + " "

#print "Executing %d copies of program: %s" % (num_procs, path)

procs = {}

for i in range(start_proc, start_proc + num_procs):
    print "Starting process: %d" % i
    env = {}
    env["CARBON_PROCESS_INDEX"] = str(i)
    env["LD_LIBRARY_PATH"] = "/afs/csail/group/carbon/tools/boost_1_38_0/stage/lib"

    procs[i] = subprocess.Popen(path, shell=True, env=env)

for i in range(start_proc, start_proc + num_procs):
    procs[i].wait()
