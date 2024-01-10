#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import signal
import time
import socket
import threading
import SocketServer

HOST = "localhost"
PORT_BEGIN = 2000
PORT_END = 2004
SocketServer.allow_reuse_address = True

exit = False

def signal_handler(num, stack):
    global exit

    print "Received signal %d, exiting..." % (num)
    exit = True

class ThreadedTCPRequestHandler(SocketServer.BaseRequestHandler):

    set_cmds = {
        "SPCH 1": "SPGS 1",
        "SPCH 2": "SPGS 2",
        "SPCH 3": "SPGS 3",
        "SPCH 6": "SPGS 6",
        "SPCH 7": "SPGS 7",
        "SPCH 8": "SPGS 8",
        "SPCH 9": "SPGS 9",
        "SPCH 10": "SPGS 10",
        "SPCH 11": "SPGS 11",
        "SPCH 12": "SPGS 12",
        "SPCH 15": "SPGS 15",
        "SPCH 26": "SPGS 26",
        "SPCH 21": "SPGS 21",
        "SPCH 23": "SPGS 23",
        "SPRP 4": "SPGP 4",
        "SPAP 4": "SPGP 4",
        "SPRP 5": "SPGP 5",
        "SPAP 5": "SPGP 5",
        "SPRP 13": "SPGP 13",
        "SPAP 13": "SPGP 13",
        "SPRP 22": "SPGP 22",
        "SPAP 22": "SPGP 22"
    }

    get_cmds = {
        "SPGS 1": 1,
        "SPGS 2": 1,
        "SPGS 3": 1,
        "SPGP 4": 1,
        "SPST 4": 1,
        "SPCA 4": 1,
        "SPGP 5": 1,
        "SPST 5": 1,
        "SPCA 5": 1,
        "SPGS 6": 1,
        "SPGS 7": 1,
        "SPGS 8": 1,
        "SPGS 9": 1,
        "SPGS 10": 1,
        "SPGS 11": 1,
        "SPGS 12": 1,
        "SPGS 15": 1,
        "SPGS 26": 1,
        "SPGS 16": 1,
        "SPGS 17": 1,
        "SPGS 21": 1,
        "SPGS 23": 1,
        "SPGP 13": 1, 
        "SPST 13": 1,
        "SPCE 14": 1,
        "SPFE 14": 1,
        "SPRE 14": 1,
        "SPCE 24": 1,
        "SPFE 24": 1,
        "SPRE 24": 1,
        "SPGP 22": 1,
        "SPST 22": 1,
        "SPCA 22": 1
    }

    def handle(self):
        global exit

        while (not exit):
            data = self.request.recv(1024)

            try:
                response = self.execute_cmd(data.strip())
            except Exception, e:
                response = "ERR %s\n" % e

            self.request.send(response)

    def execute_cmd(self, data):
        cmd_args = data.split()
        cmd = ""
        value = None

        if (len(cmd_args) == 2):
            cmd = " ".join(cmd_args)
        elif (len(cmd_args) == 3):
            cmd = " ".join(cmd_args[:2])
            value = cmd_args[2]
        else:
            return "ERR cmd_args = %s\n" % cmd_args

        print "cmd = '%s', value = '%s'" % (cmd, value)

        if ((value != None) and (self.set_cmds.has_key(cmd))):
            self.get_cmds[self.set_cmds[cmd]] = value
            return "1\n"
        elif (self.get_cmds.has_key(cmd)):
            return "%s\n" % (self.get_cmds[cmd])
        else:
            return "ERR\n"

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

def main():
    global exit

    servers = []

    signal.signal(signal.SIGINT, signal_handler)

    for port in range(PORT_BEGIN, PORT_END+1):
        server = ThreadedTCPServer((HOST, port), ThreadedTCPRequestHandler)
        servers.append(server)

        server_thread = threading.Thread(target=server.serve_forever)
        server_thread.setDaemon(True)
        server_thread.start()

    print "Starting spectrograph server..."

    while (not exit):
        time.sleep(1)

    # Python 2.6 and newer
    #for server in servers:
    #    server.shutdown()

if __name__ == "__main__":
    main()
