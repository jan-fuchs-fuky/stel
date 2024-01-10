#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
#
# $Date$
# $Rev$
# $URL$
#

import sys
import time
#import telnetlib
import socket

HOST = "192.168.193.199"
PORT = 23

def print_data(data):
    for c in data:
        print "%02X" % ord(c),
    print

class NvtSetOut():
    def __init__(self, value):
        # Warning: Telnetlib change FF => FF FF
        #tn = telnetlib.Telnet(HOST, PORT)
        #
        ## FF FA 2C 32 VALUE FF FA
        #tn.write("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x33, int(value), 0xFF, 0xFA))
        ##print tn.read_all()
        #sys.stdout.write("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x33, int(value), 0xFF, 0xFA))
        #tn.close()

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        s.sendall("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x33, int(value), 0xFF, 0xFA))
        print_data(s.recv(7))
        s.close()

def main():
    if (len(sys.argv) != 2):
        #print "Usage: %s VALUE" % sys.argv[0]

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))

        s.sendall("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x32, 0x0, 0xFF, 0xFA))
        print_data(s.recv(7))

        s.sendall("%c%c%c%c%c%c%c" % (0xFF, 0xFA, 0x2C, 0x32, 0x30, 0xFF, 0xFA))
        print_data(s.recv(7))

        s.close()

        sys.exit(0)

    NvtSetOut(sys.argv[1])

if __name__ =='__main__':
    main()
