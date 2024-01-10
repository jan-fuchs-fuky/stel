#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2022 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# http://python.net/crew/theller/ctypes/tutorial.html
# https://docs.python.org/2/library/ctypes.html
#
# $ pylint -d C,R,W -r n quido_client.py
#

import os
import sys
import time
import requests
import argparse
import logging
import sdnotify
import configparser
import multiprocessing
import ctypes
import xmlrpc.server
import xml.etree.ElementTree as ET

from ctypes import c_uint8
from logging.handlers import RotatingFileHandler

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

QUIDO_LOG = "%s/../log/quido_%%s.log" % SCRIPT_PATH
QUIDO_CFG = "%s/../etc/quido_client.cfg" % SCRIPT_PATH

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

class QuidoClient:

    def __init__(self):

        self.load_cfg()

        self.url = "http://%(host)s:%(port)i" % self.cfg["quido"]
        self.auth = (self.cfg["quido"]["user"], self.cfg["quido"]["password"])

        self.namespaces = {
            "fresh": "http://www.papouch.com/xml/quido/act",
            "set": "http://www.papouch.com/xml/quido/set",
        }

        process_name = "http_client"
        self.logger = logging.getLogger("quido_%s" % process_name)
        init_logger(self.logger, QUIDO_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(QUIDO_CFG)

        self.cfg = {
            "quido": {},
        }

        quido_callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
            "user": rcp.get,
            "password": rcp.get,
            "relays_count": rcp.getint,
        }
        self.run_cfg_callbacks("quido", quido_callbacks)

        #print(self.cfg)

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key](section, key)

    def get_status(self):
        relays = [-1] * self.cfg["quido"]["relays_count"]

        response = requests.get("%s/fresh.xml" % self.url, auth=self.auth)

        if response.status_code != 200:
            raise Exception("status_code = %i" % response.status_code)

        root = ET.fromstring(response.text)

        for dout in root.findall("fresh:dout", self.namespaces):
            idx = int(dout.attrib["id"]) - 1
            relays[idx] = int(dout.attrib["val"])

        for temp in root.findall("fresh:temp", self.namespaces):
            if temp.attrib["id"] != "1":
                raise Exception("Unknown temperature sensor (%s)" % temp.attrib["id"])

            temperature = float(temp.attrib["val"])

        return [relays, temperature]

    def set_relay(self, name, value):
        qid = name

        # 0 = r (rozepnuto), 1 = s (sepnuto), 2 = p (pulse)
        qtype = 's'

        if value == 0:
            qtype = 'r'
        elif value == 2:
            qtype = 'p'

        response = requests.get("%s/set.xml?type=%s&id=%i" % (self.url, qtype, qid), auth=self.auth)

        if response.status_code != 200:
            raise Exception("status_code = %i" % response.status_code)

        root = ET.fromstring(response.text)

        result = root.findall("set:result", self.namespaces)[0]

        if result.attrib["status"] != '1':
            raise Exception("set_relay() failed: status = %s" % result.attrib["status"])

class QuidoProcess(multiprocessing.Process):

    def __init__(self, name, events_mp, setup_mp):
        self.name = name
        self.events_mp = events_mp
        self.setup_mp = setup_mp
        super(QuidoProcess, self).__init__(name=name)

        process_name = "main"
        self.logger = logging.getLogger("quido_client_%s" % process_name)
        init_logger(self.logger, QUIDO_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.quido_client = QuidoClient()

    def loop(self):
        if (self.events_mp["set_relay"].is_set()):

            with self.setup_mp["relay_number"].get_lock():
                relay_number = self.setup_mp["relay_number"].value

            with self.setup_mp["value"].get_lock():
                value = self.setup_mp["value"].value

            self.logger.info("set_relay(relay_number=%i, value=%s)" % (relay_number, value))

            for i in range(3):
                try:
                    self.quido_client.set_relay(relay_number, value)
                    time.sleep(0.1)
                    break
                except:
                    self.logger.exception("set_relay() failed")
                    time.sleep(2)

            self.events_mp["set_relay"].clear()

        for i in range(3):
            try:
                relays, temperature = self.quido_client.get_status()
                break
            except:
                self.logger.exception("get_status() failed")
                time.sleep(2)

        with self.setup_mp["relays_state"].get_lock():
            relays_state = self.setup_mp["relays_state"]

            for idx in range(len(relays_state)):
                relays_state[idx] = relays[idx]

        with self.setup_mp["temperature"].get_lock():
            self.setup_mp["temperature"].value = temperature

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

class QuidoXmlRpcServer:

    def __init__(self):
        self.load_cfg()

        process_name = "xmlrpc"
        self.logger = logging.getLogger("quido_client_%s" % process_name)
        init_logger(self.logger, QUIDO_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.events_mp = {
            "exit": multiprocessing.Event(),
            "set_relay": multiprocessing.Event(),
        }

        self.setup_mp = {
            "relay_number": multiprocessing.Value(ctypes.c_uint8, 1),
            "value": multiprocessing.Value(ctypes.c_uint8, 1),
            "temperature": multiprocessing.Value(ctypes.c_double, 0),
            "relays_state": multiprocessing.Array(ctypes.c_int8, [-1] * self.cfg["quido"]["relays_count"]),
        }

        quido_process = QuidoProcess("QuidoProcess", self.events_mp, self.setup_mp)
        quido_process.daemon = True
        quido_process.start()

        server = xmlrpc.server.SimpleXMLRPCServer((self.cfg["server"]["host"], self.cfg["server"]["port"]))
        server.register_introspection_functions()
        server.register_function(self.rpc_set_relay, "quido_set_relay")
        server.register_function(self.rpc_get_relays, "quido_get_relays")
        server.register_function(self.rpc_get_temperature, "quido_get_temperature")

        sd_nofifier = sdnotify.SystemdNotifier()
        sd_nofifier.notify("READY=1")

        server.serve_forever()

        self.events_mp["exit"].set()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(QUIDO_CFG)

        self.cfg = {
            "server": {},
            "quido": {},
        }

        callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
        }
        self.run_cfg_callbacks("server", callbacks)

        quido_callbacks = {
            "relays_count": rcp.getint,
        }
        self.run_cfg_callbacks("quido", quido_callbacks)

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key](section, key)

    def rpc_get_temperature(self):

        with self.setup_mp["temperature"].get_lock():
            temperature = self.setup_mp["temperature"].value

        return temperature

    def rpc_get_relays(self):
        relays = [-1] * self.cfg["quido"]["relays_count"]

        with self.setup_mp["relays_state"].get_lock():
            relays_state = self.setup_mp["relays_state"]

            for idx in range(len(relays_state)):
                relays[idx] = relays_state[idx]

        return relays

    def rpc_set_relay(self, relay_number, value):

        with self.setup_mp["relay_number"].get_lock():
            self.setup_mp["relay_number"].value = relay_number

        # 0 = r (rozepnuto), 1 = s (sepnuto), 2 = p (pulse)
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
        description="QUIDO control.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=epilog)

    parser.add_argument("-d", "--daemon", action="store_true",
                        help="run as XML RPC server")

    parser.add_argument("-g", "--get", action="store_true",
                        help="Get status")

    parser.add_argument("-s", "--set", type=int, default=None, metavar="RELAY_NUMBER",
                        choices={1, 2, 3, 4, 5, 6, 7, 8},
                        help="Set RELAY_NUMBER to VALUE")

    parser.add_argument("-v", "--value", type=int, default=None, metavar="VALUE",
                        choices={0, 1},
                        help="VALUE is integer 0 (OFF) or 1 (ON)")

    args = parser.parse_args()

    if args.daemon:
        QuidoXmlRpcServer()
    elif args.get:
        quido_client = QuidoClient()
        relays, temperature = quido_client.get_status()
        print(relays, temperature)
    elif args.set is not None and args.value is not None:
        quido_client = QuidoClient()
        quido_client.set_relay(args.set, args.value)
    else:
        parser.print_help()

if __name__ == '__main__':
    main()
