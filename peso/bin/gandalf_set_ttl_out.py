#!/usr/bin/python

import sys
import time
import socket

HOST = "192.168.193.199"
PORT = 23

if (len(sys.argv) != 2):
    print("Usage: %s VALUE" % sys.argv[0])
    sys.exit()

time.sleep(1)

value = sys.argv[1]
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.sendall("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x33, int(value), 0xFF, 0xFA))
data = s.recv(7)

answer = []
for c in data:
    answer.append("%02X " % ord(c))

print("set_ttl_out(%s) => %s" % (value, "".join(answer)))
s.close()
