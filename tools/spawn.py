#!/usr/bin/env python

"""
This is the python script that is responsible for spawning a simulation
"""

import sys
import os
import subprocess

l_path = "/afs/csail/group/carbon/tools/pin/pintest/intel64/runtime"

# spawn_job:
#  start up a command on one machine
#  can be called by spawn_master.py or spawn.py
def spawn_job(proc_num, command):
   os.environ['CARBON_PROCESS_INDEX'] = "%i" % (proc_num)
   os.environ['LD_LIBRARY_PATH'] = l_path

   proc = subprocess.Popen(command, shell=True, preexec_fn=os.setsid, env=os.environ)
   return proc
