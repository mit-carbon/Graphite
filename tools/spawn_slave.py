#!/usr/bin/env python

"""
This is the slave process that receives requests over ssh to spawn a simulation
"""

import sys
import os
import subprocess
import time
import signal

import spawn
from termcolors import *

# spawn_job:
#  start up a command over an ssh connection on one machine
#  returns an object that can be passed to wait_job()
def spawn_job(proc_num, command, working_dir, graphite_home):
   exec_command = "cd %s; %s" % (working_dir, command)
   print "%s Starting process: %d: %s" % (pslave(), proc_num, exec_command)
   graphite_proc = spawn.spawn_job(proc_num, exec_command, graphite_home)
   renew_permissions_proc = spawn.spawn_renew_permissions_proc()
   return [graphite_proc, renew_permissions_proc]

# wait_job:
#  wait on a job to finish or the ssh connection to be killed
def wait_job(graphite_proc, renew_permissions_proc, proc_num):
   while True:
      # Poll the process and see if it exited
      returnCode = graphite_proc.poll()
      if returnCode != None:
         try:
            os.killpg(graphite_proc.pid, signal.SIGKILL)
         except OSError:
            pass
         print "%s Process: %d exited with ReturnCode: %d" % (pslave(), proc_num, returnCode)
         if (renew_permissions_proc != None):
            os.killpg(renew_permissions_proc.pid, signal.SIGKILL)
         return returnCode
      
      # If not, check if some the ssh connection has been killed
      # If connection killed, this becomes a child of the init process
      if (os.getppid() == 1):
         # DO NOT place a print statement here
         os.killpg(graphite_proc.pid, signal.SIGKILL)
         if (renew_permissions_proc != None):
            os.killpg(renew_permissions_proc.pid, signal.SIGKILL)
         return -1
      
      # Sleep for 0.5 seconds before checking parent pid or process status again
      time.sleep(0.5)

# get_graphite_home:
#  get the graphite home directory from the script name
def get_graphite_home(script_name):
   return (os.sep).join(script_name.split(os.sep)[:-2])

# pslave:
#  print spawn_slave.py preamble
def pslave():
    return colorstr('[spawn_slave.py]', 'BOLD')

# main -- if this is used as a standalone script
if __name__=="__main__":
  
   proc_num = int(sys.argv[2])
   command = " ".join(sys.argv[3:])
   working_dir = sys.argv[1]
   graphite_home = get_graphite_home(sys.argv[0])

   [graphite_proc, renew_permissions_proc] = spawn_job(proc_num, command, working_dir, graphite_home)
   sys.exit(wait_job(graphite_proc, renew_permissions_proc, proc_num))
