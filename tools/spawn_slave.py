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

# spawn_job:
#  start up a command over an ssh connection on one machine
#  returns an object that can be passed to wait_job()
def spawn_job(proc_num, command, working_dir):
   exec_command = "cd %s; %s" % (working_dir, command)

   print "[spawn_slave.py] Starting process: %d: %s" % (proc_num, exec_command)
   sys.stdout.flush()
   return spawn.spawn_job(proc_num, exec_command)

# wait_job:
#  wait on a job to finish or the ssh connection to be killed
def wait_job(proc, proc_num):
   while True:
      # Poll the process and see if it exited
      returnCode = proc.poll()
      if returnCode != None:
         print "[spawn_slave.py] Process: %d exited with return code: %d" % (proc_num, returnCode)
         sys.stdout.flush()
         return returnCode
      
      # If not, check if some the ssh connection has been killed
      # If connection killed, this becomes a child of the init process
      if (os.getppid() == 1):
         os.killpg(proc.pid, signal.SIGKILL)
         return -1
      
      # Sleep for 0.5 seconds before checking parent pid or process status again
      time.sleep(0.5)

# get_working_dir:
#  get the graphite working directory from the script name
def get_working_dir(script_name):
   # Get working dir from the script name
   return (os.sep).join(script_name.split(os.sep)[:-2])

# main -- if this is used as a standalone script
if __name__=="__main__":
   working_dir = get_working_dir(sys.argv[0])
   proc_num = int(sys.argv[1])
   command = " ".join(sys.argv[2:])

   proc = spawn_job(proc_num, command, working_dir)
   sys.exit(wait_job(proc, proc_num))
