#!/usr/bin/env python

"""
This is the server process that listens for spawn requests
and then spawns the simulator with the appropriate
environment variable set for that process number
"""

import socket
import subprocess

running_process_list = []
listen_port = 1999

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

    running_process_list.append(subprocess.Popen(command, shell=True, env=env))
    pass

def kill_all_processes():
    for i in range(len(running_process_list)):
        running_process_list[i].kill()

def handle_command(data, client):
    print "received data from client.", data
    if data[0] == 's':
        print "got spawn command: ", data
        spawn_args = data[1:].split(",")
        process_number = int(spawn_args[0])
        process_command = spawn_args[1]
        spawn_process(process_number, process_command)
        client.send("ack")
    elif data[0] == 'c' and len(data) == 1:
        print "got close all command."
        kill_all_processes()
        client.send("ack")
    else:
        print "got unknown command."
        client.send("nack")


def verify_client_allowed(client):
    #FIXME!!
    return True

server = start_server()

while 1:
    client, address = server.accept()
    if not verify_client_allowed(client):
        client.close()
        continue

    size = 1024
    data = client.recv(size)
    print "client accepted connection."
    if data:
        handle_command(data, client)
    client.close()
    print "terminating connection"
