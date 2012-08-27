#!/usr/bin/env python

"""
This is the python script that is responsible for spawning a simulation
"""

import sys
import os
import subprocess

# spawn_job:
#  start up a command on one machine
#  can be called by spawn_master.py or spawn.py

def spawn_job(proc_num, command, graphite_home):
#Get PIN_HOME from Makefile.config and set LD_LIBRARY_PATH
   p = subprocess.Popen(("grep" , "-w", "PIN_HOME", graphite_home + "/Makefile.config"), stdout = subprocess.PIPE)
   lbl = p.communicate()[0]
   lpath = lbl.split('=')[1]
   lpath = lpath.strip();
   lpath = lpath+'/intel64/runtime'
   os.environ['LD_LIBRARY_PATH'] =  lpath
   os.environ['CARBON_PROCESS_INDEX'] = "%d" % (proc_num)
   proc = subprocess.Popen(command, shell=True, preexec_fn=os.setsid, env=os.environ)
   return proc
