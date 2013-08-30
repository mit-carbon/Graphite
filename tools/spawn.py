#!/usr/bin/env python

"""
This is the python script that is responsible for spawning a simulation
"""

import sys
import os
import commands
import subprocess

from termcolors import *

# get GRAPHITE_HOME from environment variable, or use pwd
def get_graphite_home():
    graphite_home = os.environ.get('GRAPHITE_HOME')
    if graphite_home == None:
        cwd = os.getcwd()
        warning_msg = "GRAPHITE_HOME undefined. Setting GRAPHITE_HOME to %s" % (cwd)
        print "\n%s" % (pWARNING(warning_msg))
        return cwd

    return graphite_home

# get PIN_HOME from Makefile.config
def get_pin_home(graphite_home):
    return commands.getoutput("grep '^\s*PIN_HOME' %s/Makefile.config | sed 's/^\s*PIN_HOME\s*=\s*\(.*\)/\\1/'" % (graphite_home))

# spawn_job:
#  start up a command on one machine
#  can be called by spawn_master.py or spawn_slave.py

def spawn_job(proc_num, command, graphite_home):
    # Set LD_LIBRARY_PATH using PIN_HOME from Makefile.config
    os.environ['LD_LIBRARY_PATH'] =  "%s/intel64/runtime" % get_pin_home(graphite_home)
    os.environ['CARBON_PROCESS_INDEX'] = "%d" % (proc_num)
    os.environ['GRAPHITE_HOME'] = graphite_home
    proc = subprocess.Popen(command, shell=True, preexec_fn=os.setsid, env=os.environ)
    return proc

# spawn_renew_permissions_proc:
#  command to renew Kerberos/AFS tokens
def spawn_renew_permissions_proc():
   try:
      test_proc = subprocess.Popen("krenew")
   except OSError:
      return None
   test_proc.wait()
   return subprocess.Popen("krenew -K 60 -t", shell=True, preexec_fn=os.setsid)
