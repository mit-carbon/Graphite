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
def spawn_job(proc_num, command):
   os.environ['CARBON_PROCESS_INDEX'] = "%d" % (proc_num)

   proc = subprocess.Popen(command, shell=True, preexec_fn=os.setsid, env=os.environ)
   return proc
