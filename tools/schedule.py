#!/usr/bin/env python

# This takes a list of machines and a list of jobs and greedily runs
# them on the machines as they become available.

import os
import time
import shutil
import subprocess
import signal
import random

from termcolors import *

# Job:
#  a simple struct to hold data for a job
#  different subclasses of Job specialize for different types of commands
class Job:
    def __init__(self, num_machines, command):
        self.num_machines = num_machines
        self.command = command

    def spawn(self, wd):
        pass

    def poll(self):
        pass

    def wait(self):
        pass

    def kill(self):
        pass

# SpawnJob:
#  just run something locally but consume lots of machines to do so
class SpawnJob(Job):
    def __init__(self, num_machines, command):
        Job.__init__(self, num_machines, command)

    def spawn(self):
        self.proc = subprocess.Popen(self.command, shell=True, preexec_fn=os.setsid)

    def poll(self):
        return self.proc.poll()

    def wait(self):
        return self.proc.wait()

    def kill(self):
        return os.killpg(self.proc.pid, signal.SIGINT)

# MakeJob:
#  a job built around the make system
class MakeJob(SpawnJob):

    def __init__(self, num_machines, command, config_filename, results_dir, sub_dir, sim_flags, app_flags, mode="pin"):
        SpawnJob.__init__(self, num_machines, command)
        self.config_filename = config_filename
        self.results_dir = results_dir
        self.sub_dir = "%s/%s" % (results_dir, sub_dir)
        self.sim_flags = sim_flags
        self.app_flags = app_flags
        self.mode = mode

    def make_command(self):
        self.sim_flags += " -c %s/%s --general/output_dir=%s/%s --general/num_processes=%d --transport/base_port=%d" % \
              (os.getcwd(), self.config_filename, os.getcwd(), self.sub_dir, len(self.machines), random.randint(2000,15000))
        for i in range(0,len(self.machines)):
            self.sim_flags += " --process_map/process%d=%s" % (i, self.machines[i])
       
        self.command += " SIM_FLAGS=\"%s\"" % (self.sim_flags)
        if self.app_flags != None:
            self.command += " APP_FLAGS=\"%s\"" % (self.app_flags)
        self.command += " > %s/output 2>&1" % (self.sub_dir)

    def spawn(self):

        # make output directory
        try:
            os.makedirs(self.sub_dir)
        except OSError:
            pass

        # format command
        self.make_command()
        print "%s %s" % (pschedule(), self.command)

        SpawnJob.spawn(self)

# schedule - run a list of jobs across some machines
def schedule(machine_list, jobs):

    # initialization
    available = machine_list
    running = []

    # helpers
    def get_job(machine_list, jobs):
        for i in range(0,len(jobs)):
            if jobs[i].num_machines <= len(available):
                j = jobs[i]
                del jobs[i]
                j.machines = available[0:j.num_machines]
                del available[0:j.num_machines]
                del j.num_machines
                return j
        return

    try:
        # main loop
        while True:

            # spawn another job, if available
            job = get_job(available, jobs)

            if job != None:
                job.spawn()
                running.append(job)

            if len(jobs) == 0:
                break

            # check active jobs
            terminated = []
            for i in range(0,len(running)):
                status = running[i].poll()
                if status != None:
                    available.extend(running[i].machines)
                    terminated.append(i)

            terminated.reverse()
            for i in terminated:
                del running[i]
                    
            time.sleep(0.1)

        # wait on remaining
        for job in running:
            job.wait()

    except KeyboardInterrupt:

        # Kill all jobs
        for job in running:
            msg = colorstr('Keyboard interrupt. Killing simulation', 'RED')
            print "%s %s: %s" % (pschedule(), msg, job.command)
            job.kill()

# pschedule:
#  print the schedule.py preamble
def pschedule():
    return colorstr('[schedule.py]', 'BOLD')

