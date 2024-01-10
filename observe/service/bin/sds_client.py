#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2020 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# http://python.net/crew/theller/ctypes/tutorial.html
# https://docs.python.org/2/library/ctypes.html
#
# http://wiki.merenienergie.cz/index.php/UDP_protocol#write.2C_rdsys.2C_rdram.2C_wrsys.2C_wrram
# http://wiki.merenienergie.cz/index.php/Sdsc_sysp#Provozn.C3.AD_.C3.BAdaje
#
# $ pylint -d C,R,W -r n sds_client.py
#

import os
import sys
import time
import socket
import argparse
import logging
import sdnotify
import configparser
import multiprocessing
import ctypes
import xmlrpc.server

from ctypes import c_uint8
from logging.handlers import RotatingFileHandler

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

SDS_LOG = "%s/../log/sds_%%s.log" % SCRIPT_PATH
SDS_CFG = "%s/../etc/sds_client.cfg" % SCRIPT_PATH

SDS_RELAY_RDSYS = [
    75,
    83,
    91,
    99,
    107,
    115,
]

def init_logger(logger, filename):
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - %(message)s")

    # DBG
    #formatter = logging.Formatter(
    #    ("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - "
    #     "%(filename)s:%(lineno)s - %(funcName)s() - %(message)s - "))

    fh = RotatingFileHandler(filename, maxBytes=10485760, backupCount=10)
    fh.setLevel(logging.INFO)
    fh.setFormatter(formatter)

    logger.setLevel(logging.INFO)
    logger.addHandler(fh)

class SdsClient:

    def __init__(self):

        self.load_cfg()

        self.relay_name2wrsys = {
            1: 231,
            2: 232,
            3: 233,
            4: 234,
            5: 235,
            6: 236,
        }

        self.sds_header = [
            0x00,
            'A',
            'N',
            '-',
            'D',
            '.',
            'c',
            'z',
            '/',
            'S',
            'D',
            'S',
            0x02,
            0x06,
            0x00,
        ]

        self.sds_header_len = len(self.sds_header)

        process_name = "udp_client"
        self.logger = logging.getLogger("sds_%s" % process_name)
        init_logger(self.logger, SDS_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(SDS_CFG)

        self.cfg = {
            "sds": {},
        }

        sds_callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
            "password": rcp.get,
        }
        self.run_cfg_callbacks("sds", sds_callbacks)

        #print(self.cfg)

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key](section, key)

    def get_relays(self):
        relays = [-1] * len(SDS_RELAY_RDSYS)

        packet_size = 116

        sds_rdsys = list(self.sds_header) + ((packet_size - self.sds_header_len) * [0])
        sds_rdsys[15] = 'r'
        sds_rdsys[16] = 'd'
        sds_rdsys[17] = 's'
        sds_rdsys[18] = 'y'
        sds_rdsys[19] = 's'

        # delka hesla: 28, 29, 30, 31
        sds_rdsys[31] = len(self.cfg["sds"]["password"])

        # heslo: 32, .., 63 (nepouzite znaky 0x00)
        # nad kazdym znakem hesla je treba udelat XOR vuci konstante 0xa5
        idx = 32
        for c in self.cfg["sds"]["password"]:
            sds_rdsys[idx] = ord(c) ^ 0xa5
            idx += 1

        # pocet nasledujicich ulozenych paru: 64, 65, 66, 67
        sds_rdsys[67] = len(SDS_RELAY_RDSYS)

        for idx in range(len(SDS_RELAY_RDSYS)):
            position = 71 + (8 * idx)
            sys_address = 231 + idx
            sds_rdsys[position] = sys_address

        sds_packet_rdsys = self.data2packet(sds_rdsys, packet_size)
        self.print_packet(sds_packet_rdsys, "RDSYS packet")

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)
        server_address = (self.cfg["sds"]["host"], self.cfg["sds"]["port"])

        try:
            sent = sock.sendto(sds_packet_rdsys, server_address)
            data, server = sock.recvfrom(4096)

            self.print_packet(data, "RDSYS recvfrom")

            if len(data) != 116:
                print("ERROR:")

            print("data_len = %i" % len(data))

            for idx in range(len(SDS_RELAY_RDSYS)):
                print("RELAY%i = %i" % (idx+1, data[SDS_RELAY_RDSYS[idx]]))
        finally:
            print('closing socket')
            sock.close()

        for idx in range(len(SDS_RELAY_RDSYS)):
            value = data[SDS_RELAY_RDSYS[idx]]
            if value > 0:
                value = 1
            relays[idx] = value

        return relays

    def set_relay(self, name, value):
        if (value > 0):
            value = 255

        packet_size = 76

        sds_wrsys = list(self.sds_header) + ((packet_size - self.sds_header_len) * [0])
        sds_wrsys[15] = 'w'
        sds_wrsys[16] = 'r'
        sds_wrsys[17] = 's'
        sds_wrsys[18] = 'y'
        sds_wrsys[19] = 's'

        # delka hesla: 28, 29, 30, 31
        sds_wrsys[31] = len(self.cfg["sds"]["password"])

        # heslo: 32, .., 63 (nepouzite znaky 0x00)
        # nad kazdym znakem hesla je treba udelat XOR vuci konstante 0xa5
        idx = 32
        for c in self.cfg["sds"]["password"]:
            sds_wrsys[idx] = ord(c) ^ 0xa5
            idx += 1

        # pocet nasledujicich ulozenych paru: 64, 65, 66, 67
        sds_wrsys[67] = 1

        # zacatek paru: 68
        # par: 2x 4 byty
        # rele 1, bin(231) = 1110 0111
        # index: 68, 69, 70, 71
        sds_wrsys[71] = self.relay_name2wrsys[name]

        # bin(255) = 1111 1111
        # hodnota: 72, 73, 74, 75
        sds_wrsys[75] = value

        sds_packet_wrsys = self.data2packet(sds_wrsys, packet_size)
        self.print_packet(sds_packet_wrsys, "WRSYS packet")

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)
        server_address = (self.cfg["sds"]["host"], self.cfg["sds"]["port"])

        try:
            sent = sock.sendto(sds_packet_wrsys, server_address)
            data, server = sock.recvfrom(4096)

            self.print_packet(data, "WRSYS recvfrom")
            self.compare_packets(sds_packet_wrsys[:20], data[:20])

            answer_wrsys = data[71]
            answer_value = data[75]

            if answer_wrsys != self.relay_name2wrsys[name]:
                print("ERROR %s != %s" % (answer_wrsys, self.relay_name2wrsys[name]))

            if answer_value != value:
                print("ERROR %s != %s" % (answer_value, value))

            print("WRSYS %i = %i success" % (answer_wrsys, answer_value))
        finally:
            print('closing socket')
            sock.close()

    def print_packet(self, packet, description):
        self.logger.debug(description)

        for i in range(len(packet)):
            item = packet[i]

            c = ""
            if item >= 0 and item <= 127:
                c = chr(item)
                if not c.isprintable():
                    c = ""

            self.logger.debug("%04i. 0x%02x (%03i) %s" % (i, item, item, c))

    def compare_packets(self, packet1, packet2):
        for idx, item in enumerate(packet1):
            if packet2[idx] != item:
                print("ERROR")
                break

    def data2packet(self, data, packet_size):
        CUInt8Array = c_uint8 * packet_size
        packet = CUInt8Array()

        for i in range(len(data)):
            item = data[i]
            if (isinstance(item, int)):
                packet[i] = item
            else:
                packet[i] = ord(item)

        return packet

    def ascii2int(self, values, length):
        number = 0

        for idx in range(length):
            position = 8 * ((length - 1) - idx)
            byte = values[idx] << position
            number += byte

        if number > 0x80000000:
            number -= 0x100000000

        return number

    # TODO: zjistit proc vraci pouze stav prvnich dvou rele
    #def query(self):
    #    packet_size = 21

    #    sds_query = list(self.sds_header) + ((packet_size - self.sds_header_len) * [0])
    #    sds_query[15] = 'q'
    #    sds_query[16] = 'u'
    #    sds_query[17] = 'e'
    #    sds_query[18] = 'r'
    #    sds_query[19] = 'y'

    #    sds_packet_query = self.data2packet(sds_query, packet_size)

    #    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #    server_address = ("192.168.1.250", 280)

    #    try:
    #        sent = sock.sendto(sds_packet_query, server_address)

    #        print('waiting to receive')
    #        data, server = sock.recvfrom(4096)
    #        #print >>sys.stderr, '%s received "%s"' % (server, data)

    #        print(len(data))
    #        for i in range(len(data)):
    #            if True:
    #            # relay
    #            #if i > 106 and i < 235:
    #            # temp
    #            #if i >= 403 and i <= 420:
    #                item = data[i]
    #                print("%04i 0x%02x %s" % (i, item, chr(item)))

    #        # stav rele 1 => 107, 108, 109, 110
    #        # stav rele 2 => 111, 112, 113, 114
    #        for idx in range(1, 7):
    #            address = 106 + (4 * idx)
    #            print("%i. RELAY (%i) = %i" % (idx, address, data[address]))

    #        # 0x000007D0 = +2000
    #        # 0xFFFFF830 = -2000
    #        #temp = self.ascii2int([chr(0xff), chr(0xff), chr(0xf8), chr(0x30)], 4)
    #        temp = self.ascii2int(data[403+8:403+8+4], 4)
    #        temp *= 0.01
    #        print("temp = %.2f" % temp)
    #    finally:
    #        print('closing socket')
    #        sock.close()

class SdsProcess(multiprocessing.Process):

    def __init__(self, name, events_mp, setup_mp):
        self.name = name
        self.events_mp = events_mp
        self.setup_mp = setup_mp
        super(SdsProcess, self).__init__(name=name)

        process_name = "main"
        self.logger = logging.getLogger("sds_client_%s" % process_name)
        init_logger(self.logger, SDS_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.sds_client = SdsClient()

    def loop(self):
        if (self.events_mp["set_relay"].is_set()):

            with self.setup_mp["relay_number"].get_lock():
                relay_number = self.setup_mp["relay_number"].value

            with self.setup_mp["value"].get_lock():
                value = self.setup_mp["value"].value

            self.logger.info("set_relay(relay_number=%i, value=%s)" % (relay_number, value))

            # TODO: sdilet kod s cyklem get_relays()
            for i in range(3):
                try:
                    self.sds_client.set_relay(relay_number, value)
                    time.sleep(0.1)
                    break
                except socket.timeout:
                    self.logger.exception("set_relay() socket.timeout")
                    time.sleep(2)

            self.events_mp["set_relay"].clear()

        for i in range(3):
            try:
                relays = self.sds_client.get_relays()
                break
            except socket.timeout:
                self.logger.exception("get_relays() socket.timeout")
                time.sleep(2)

        with self.setup_mp["relays_state"].get_lock():
            relays_state = self.setup_mp["relays_state"]

            for idx in range(len(relays_state)):
                relays_state[idx] = relays[idx]

    def run(self):
        sd_nofifier = sdnotify.SystemdNotifier()

        while (not self.events_mp["exit"].is_set()):
            sd_nofifier.notify("WATCHDOG=1")

            try:
                self.loop()
                time.sleep(1)
            except:
                self.logger.exception("loop() exception")
                time.sleep(15)

class SdsXmlRpcServer:

    def __init__(self):
        self.load_cfg()

        process_name = "xmlrpc"
        self.logger = logging.getLogger("sds_client_%s" % process_name)
        init_logger(self.logger, SDS_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.events_mp = {
            "exit": multiprocessing.Event(),
            "set_relay": multiprocessing.Event(),
        }

        self.setup_mp = {
            "relay_number": multiprocessing.Value(ctypes.c_uint8, 1),
            "value": multiprocessing.Value(ctypes.c_uint8, 1),
            "relays_state": multiprocessing.Array(ctypes.c_int8, [-1] * len(SDS_RELAY_RDSYS)),
        }

        sds_process = SdsProcess("SdsProcess", self.events_mp, self.setup_mp)
        sds_process.daemon = True
        sds_process.start()

        server = xmlrpc.server.SimpleXMLRPCServer((self.cfg["server"]["host"], self.cfg["server"]["port"]))
        server.register_introspection_functions()
        server.register_function(self.rpc_set_relay, "sds_set_relay")
        server.register_function(self.rpc_get_relays, "sds_get_relays")

        sd_nofifier = sdnotify.SystemdNotifier()
        sd_nofifier.notify("READY=1")

        server.serve_forever()

        self.events_mp["exit"].set()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(SDS_CFG)

        self.cfg = {
            "server": {},
        }

        callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
        }
        self.run_cfg_callbacks("server", callbacks)

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key](section, key)

    def rpc_get_relays(self):
        relays = [-1] * len(SDS_RELAY_RDSYS)

        with self.setup_mp["relays_state"].get_lock():
            relays_state = self.setup_mp["relays_state"]

            for idx in range(len(relays_state)):
                relays[idx] = relays_state[idx]

        return relays

    def rpc_set_relay(self, relay_number, value):

        with self.setup_mp["relay_number"].get_lock():
            self.setup_mp["relay_number"].value = relay_number

        with self.setup_mp["value"].get_lock():
            self.setup_mp["value"].value = value

        self.events_mp["set_relay"].set()

        return True

def main():

    epilog = (
        "Example1 (set RELAY1 to OFF):\n\n    %s -s 1 -v 0\n\n"
        "Example2 (get relays status):\n\n    %s -g\n\n"
        "Example3 (run as XML RPC server):\n\n    %s -d\n\n" % (sys.argv[0], sys.argv[0], sys.argv[0])
    )

    parser = argparse.ArgumentParser(
        description="SDS Micro DIN IO 6 control.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=epilog)

    parser.add_argument("-d", "--daemon", action="store_true",
                        help="run as XML RPC server")

    parser.add_argument("-g", "--get", action="store_true",
                        help="Get relays status")

    parser.add_argument("-s", "--set", type=int, default=None, metavar="RELAY_NUMBER",
                        choices={1, 2, 3, 4, 5, 6},
                        help="Set RELAY_NUMBER to VALUE")

    parser.add_argument("-v", "--value", type=int, default=None, metavar="VALUE",
                        choices={0, 1},
                        help="VALUE is integer 0 (OFF) or 1 (ON)")

    args = parser.parse_args()

    if args.daemon:
        SdsXmlRpcServer()
    elif args.get:
        sds_client = SdsClient()
        sds_client.get_relays()
    elif args.set is not None and args.value is not None:
        sds_client = SdsClient()
        sds_client.set_relay(args.set, args.value)
    else:
        parser.print_help()

if __name__ == '__main__':
    main()
