#!/usr/bin/env python

"""
This is the server process that listens for spawn requests
and then spawns the simulator with the appropriate
environment variable set for that process number
"""

import socket
import subprocess
import sys
import os
import signal
import threading

running_process_list = []
listen_port = 1999

allowed_hosts = ["127.0.0.1", "cagnode1", "cagnode2",
        "cagnode3", "cagnode4", "cagnode5", "cagnode6", "cagnode7",
        "cagnode8", "cagnode9", "cagnode10", "cagnode11", "cagnode12",
        "cagnode13", "cagnode14", "cagnode15", "cagnode16", "cagnode17",
        "cagnode18"]

resolved_allowed_hosts = []

class SimWatcher( threading.Thread ):
    def run( self ):
        self.process.wait()
        try:
            self.socket.send("ack")
            client.close()
            print "terminating connection."
        except:
            print "unknown error sending ack / terminating connection."

    def __init__ (self, process, socket):
        self.process = process
        self.socket = socket
        threading.Thread.__init__(self)

def start_server():
    host = ''
    backlog = 5
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host,listen_port))
    s.listen(backlog)
    return s

def spawn_process(number,command):
    print "Starting process: %d" % number
    env = {}
    env["CARBON_PROCESS_INDEX"] = str(number)
    env["LD_LIBRARY_PATH"] = "/afs/csail/group/carbon/tools/boost_1_38_0/stage/lib"

    subproc = subprocess.Popen(command, shell=True, env=env)
    running_process_list.append(subproc)
    return subproc

def kill_all_processes():
    global running_process_list
    for i in range(len(running_process_list)):
        print "Killing process: %d" % running_process_list[i].pid
        os.kill(running_process_list[i].pid, signal.SIGKILL)
    running_process_list = []


def handle_command(data, client):
    if data[0] == 's':
        print "got spawn command: ", data
        spawn_args = data[1:].split(",")
        process_number = int(spawn_args[0])
        process_command = spawn_args[1]
        process = spawn_process(process_number, process_command)

        # Create a thread to watch the spawned process
        SimWatcher(process, client).start()

    elif data[0] == 'c' and len(data) == 1:
        print "got close all command."
        kill_all_processes()
        client.send("ack")
        client.close()
        print "terminating connection"
    else:
        print "got unknown command."
        client.send("nack")
        client.close()
        print "terminating connection"

try:
    allowed_hosts = map(lambda x: socket.gethostbyname(x), allowed_hosts)
except:
    print "Unexpected error:", sys.exc_info()[0]
    raise

server = start_server()

while 1:
    client, address = server.accept()
    if allowed_hosts.count(address) > 0:
        client.close()
        continue

    size = 1024
    data = client.recv(size)
    print "client accepted connection."
    if data:
        handle_command(data, client)
