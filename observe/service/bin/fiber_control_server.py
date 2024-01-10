#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2020 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import os
import sys
import time
import ctypes
import serial
import traceback
import sdnotify
import xmlrpc.server
import multiprocessing
import logging

from ctypes import c_uint8
from logging.handlers import RotatingFileHandler

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

FIBER_CONTROL_SERVER_LOG = "%s/../log/fiber_control_server_%%s.log" % SCRIPT_PATH

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

class SerialProcess(multiprocessing.Process):

    def __init__(self, name, exit_event_mp, lock_mp, values_mp, events_mp, setup_mp):
        self.name = name
        self.exit_event_mp = exit_event_mp
        self.lock_mp = lock_mp
        self.values_mp = values_mp
        self.events_mp = events_mp
        self.setup_mp = setup_mp
        super(SerialProcess, self).__init__(name=name)

        process_name = "serial"
        self.logger = logging.getLogger("fiber_control_server_%s" % process_name)
        init_logger(self.logger, FIBER_CONTROL_SERVER_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

    def set_axis_position(self, axis, value):
        byte1 = value >> 8
        byte2 = value & 0x00FF

        ByteArray4 = c_uint8 * 4
        cmd = ByteArray4()
        cmd[0] = ord('M')
        cmd[1] = ord(axis)
        cmd[2] = byte1
        cmd[3] = byte2

        self.write_cmd(cmd)
        result = self.get_answer(-1, [0x44, 0x45])

        answer = -1
        if (result):
            answer = result[0]

        self.logger.info("set_axis_position 0x%X 0x%X => 0x%X" % (byte1, byte2, answer))

    def inc_axis_position(self, axis, direction):
        ByteArray3 = c_uint8 * 3
        cmd = ByteArray3()
        cmd[0] = ord('S')
        cmd[1] = ord(axis)
        cmd[2] = ord(direction)

        self.write_cmd(cmd)
        result = self.get_answer(-1, [0x44, 0x45])

        answer = -1
        if (result):
            answer = result[0]

        self.logger.info("inc_axis_position %s %s => 0x%X" % (axis, direction, answer))

    def set_camera_power(self, value):
        ByteArray2 = c_uint8 * 2
        cmd = ByteArray2()
        cmd[0] = ord('C')
        cmd[1] = ord(str(value))

        self.write_cmd(cmd)
        result = self.get_answer(-1, [0x44])

        answer = -1
        if (result):
            answer = result[0]

        self.logger.info("set_camera_power %i => 0x%X" % (value, answer))

    def toptec_reset(self):
        ByteArray2 = c_uint8 * 2
        cmd = ByteArray2()
        cmd[0] = ord('R')
        cmd[1] = ord('R')

        self.write_cmd(cmd)
        result = self.get_answer(-1, [0xE0])

        answer = -1
        if (result):
            answer = result[0]

        self.logger.info("reset => 0x%X" % (answer))
        time.sleep(10)

    def setup(self, key):
        self.logger.info("Setup event %s" % key)

        if (key == "set_focus_position"):
            with self.setup_mp["focus_position"].get_lock():
                value = self.setup_mp["focus_position"].value
            self.set_axis_position('1', value)
        elif (key == "set_camera_position"):
            with self.setup_mp["cameras_position"].get_lock():
                value = self.setup_mp["cameras_position"].value
            self.set_axis_position('2', value)
        elif (key == "set_camera_power"):
            with self.setup_mp["cameras_power"].get_lock():
                value = self.setup_mp["cameras_power"].value
            self.set_camera_power(value)
        elif (key == "dec_focus_position"):
            self.inc_axis_position('1', '-')
        elif (key == "inc_focus_position"):
            self.inc_axis_position('1', '+')
        elif (key == "dec_camera_position"):
            self.inc_axis_position('2', '-')
        elif (key == "inc_camera_position"):
            self.inc_axis_position('2', '+')
        elif (key == "reset"):
            self.toptec_reset()

    def process_events(self):
        for key in self.events_mp:
            if (self.events_mp[key].is_set()):
                self.setup(key)
                self.events_mp[key].clear()

    def loop(self):
        self.process_events()

        # TODO: pridavat timestamp
        values = {}

        # TODO: zpracovat chybu 0xFF
        self.write_cmd(self.sa_cmd)
        result = self.get_answer(8)

        # 0  1  2  3  4  5  6  7
        # SB V0 V1 V2 P1    P2
        # 0F 20 32 88 01 FF 3A 65
        status = self.bits2dict(result[0])
        values.update(status)

        keys = [ "voltage_33", "voltage_50", "voltage_120" ]
        for idx in range(3):
            values[keys[idx]] = result[idx+1]

        values["focus_position"] = self.two_bytes2int(result[4:6])
        values["cameras_position"] = self.two_bytes2int(result[6:])

        #self.write_cmd(self.p1_cmd)
        #result = self.get_answer(2)
        #values["focus_position"] = self.two_bytes2int(result)
        #time.sleep(0.1)

        #self.write_cmd(self.p2_cmd)
        #result = self.get_answer(2)
        #values["cameras_position"] = self.two_bytes2int(result)
        #time.sleep(0.1)

        #self.write_cmd(self.sb_cmd)

        #result = self.get_answer(1)
        #if (not result):
        #    raise Exception("status bit not read")
        #time.sleep(0.1)

        #status = self.bits2dict(result[0])
        #values.update(status)

        #keys = [ "voltage_33", "voltage_50", "voltage_120" ]
        #for idx in range(3):
        #    self.write_cmd(self.voltage_cmds[idx])
        #    result = self.get_answer(1)
        #    if (not result):
        #        self.logger.warning("%s not read" % keys[idx])
        #        return
        #    time.sleep(0.1)
        #    values[keys[idx]] = result[0]

        self.lock_mp.acquire()

        for key in values:
            self.values_mp[key].value = values[key]

        self.lock_mp.release()
        #print(values)

    def run(self):
        ByteArray2 = c_uint8 * 2

        self.p1_cmd = ByteArray2()
        self.p1_cmd[0] = ord('P')
        self.p1_cmd[1] = ord('1')

        self.p2_cmd = ByteArray2()
        self.p2_cmd[0] = ord('P')
        self.p2_cmd[1] = ord('2')

        self.sb_cmd = ByteArray2()
        self.sb_cmd[0] = ord('S')
        self.sb_cmd[1] = ord('B')

        self.sa_cmd = ByteArray2()
        self.sa_cmd[0] = ord('S')
        self.sa_cmd[1] = ord('A')

        self.voltage_cmds = []
        for idx in range(3):
            cmd = ByteArray2()
            cmd[0] = ord('V')
            cmd[1] = ord(str(idx))
            self.voltage_cmds.append(cmd)

        self.hw_serial = serial.Serial("/dev/ttyS2", 9600, timeout=50)
        sd_nofifier = sdnotify.SystemdNotifier()

        while (not self.exit_event_mp.is_set()):
            sd_nofifier.notify("WATCHDOG=1")

            try:
                self.loop()
                time.sleep(0.5)
            except:
                self.logger.exception("loop() exception")
                time.sleep(4)
                try:
                    self.get_answer(32)
                except:
                    self.logger.exception("get_answer() exception")
                time.sleep(1)

        self.hw_serial.close()

    def two_bytes2int(self, bytes_array):
        number = -1

        if (len(bytes_array) != 2):
            raise Exception("two_bytes2int() failed: bytes_array != 2")

        byte1 = bytes_array[0] << 8
        byte2 = bytes_array[1]
        number = byte1 + byte2

        return number

    def bits2dict(self, byte):
        status = {}
        status["esw1a"] = not bool(byte & 0b00000001)
        status["esw1b"] = not bool((byte >> 1) & 0b00000001)
        status["esw2a"] = not bool((byte >> 2) & 0b00000001)
        status["esw2b"] = not bool((byte >> 3) & 0b00000001)
        status["g1"] = bool((byte >> 4) & 0b00000001)
        status["g2"] = bool((byte >> 5) & 0b00000001)

        return status

    def write_cmd(self, cmd):
        counter = 1
        for byte in cmd:
            self.logger.debug("CMD %i. byte 0x%X %s %i" % (counter, byte, bin(byte), byte))
            counter += 1
        self.logger.debug("CMD END")

        self.hw_serial.write(cmd)

    def get_answer(self, result_size, expected_answer=[]):
        result = []

        success = False
        for i in range(32):
            answer = self.hw_serial.read(size=1)
            if (not answer):
                self.logger.warning("hw_serial.read() => %s" % answer)
                break

            for byte in answer:
                result.append(byte)

                if (byte in expected_answer):
                    success = True
                    break

            if (len(result) == result_size) or success:
                break

        counter = 1
        for byte in result:
            self.logger.debug("ANSWER %i. byte 0x%X %s %i" % (counter, byte, bin(byte), byte))
            counter += 1
        self.logger.debug("ANSWER END")

        return result

class FiberControlServer:

    def __init__(self):
        #server = SimpleXMLRPCServer(
        #    (self.main_host, self.main_port), requestHandler=RequestHandler)

        process_name = "main"
        self.logger = logging.getLogger("fiber_control_server_%s" % process_name)
        init_logger(self.logger, FIBER_CONTROL_SERVER_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.lock_mp = multiprocessing.Lock()

        self.values_mp = {
            "focus_position": multiprocessing.Value(ctypes.c_uint32, 0),
            "cameras_position": multiprocessing.Value(ctypes.c_uint32, 0),
            "voltage_33": multiprocessing.Value(ctypes.c_uint32, 0),
            "voltage_50": multiprocessing.Value(ctypes.c_uint32, 0),
            "voltage_120": multiprocessing.Value(ctypes.c_uint32, 0),
            "g1": multiprocessing.Value(ctypes.c_bool, False),
            "g2": multiprocessing.Value(ctypes.c_bool, False),
            "esw1a": multiprocessing.Value(ctypes.c_bool, False),
            "esw1b": multiprocessing.Value(ctypes.c_bool, False),
            "esw2a": multiprocessing.Value(ctypes.c_bool, False),
            "esw2b": multiprocessing.Value(ctypes.c_bool, False),
        }

        self.setup_mp = {
            "cameras_power": multiprocessing.Value(ctypes.c_uint16, 0),
            "cameras_position": multiprocessing.Value(ctypes.c_uint16, 0),
            "focus_position": multiprocessing.Value(ctypes.c_uint16, 0),
        }

        self.events_mp = {
            "set_camera_power": multiprocessing.Event(),
            "set_camera_position": multiprocessing.Event(),
            "dec_camera_position": multiprocessing.Event(),
            "inc_camera_position": multiprocessing.Event(),
            "set_focus_position": multiprocessing.Event(),
            "dec_focus_position": multiprocessing.Event(),
            "inc_focus_position": multiprocessing.Event(),
            "reset": multiprocessing.Event(),
        }

        self.exit_event_mp = multiprocessing.Event()

        serial_process = SerialProcess("SerialProcess",
            self.exit_event_mp, self.lock_mp, self.values_mp, self.events_mp, self.setup_mp)

        serial_process.daemon = True
        serial_process.start()

        server = xmlrpc.server.SimpleXMLRPCServer(("0.0.0.0", 6000))
        server.register_introspection_functions()
        server.register_function(self.rpc_get_values, "toptec_get_values")
        server.register_function(self.rpc_set_camera_power, "toptec_set_camera_power")
        server.register_function(self.rpc_set_camera_position, "toptec_set_camera_position")
        server.register_function(self.rpc_dec_camera_position, "toptec_dec_camera_position")
        server.register_function(self.rpc_inc_camera_position, "toptec_inc_camera_position")
        server.register_function(self.rpc_set_focus_position, "toptec_set_focus_position")
        server.register_function(self.rpc_dec_focus_position, "toptec_dec_focus_position")
        server.register_function(self.rpc_inc_focus_position, "toptec_inc_focus_position")
        server.register_function(self.rpc_reset, "toptec_reset")

        sd_nofifier = sdnotify.SystemdNotifier()
        sd_nofifier.notify("READY=1")

        server.serve_forever()

        self.exit_event_mp.set()

    def rpc_get_values(self):
        values = {}

        self.lock_mp.acquire()

        for key in self.values_mp:
            #print(key)
            values[key] = self.values_mp[key].value

        self.lock_mp.release()

        return values

    def rpc_set_camera_power(self, value):
        self.logger.info("set_camera_power %s" % value)

        with self.setup_mp["cameras_power"].get_lock():
            self.setup_mp["cameras_power"].value = value
        self.events_mp["set_camera_power"].set()

        return True

    def rpc_set_camera_position(self, value):
        self.logger.info("set_camera_position %s" % value)

        with self.setup_mp["cameras_position"].get_lock():
            self.setup_mp["cameras_position"].value = value

        self.events_mp["set_camera_position"].set()

        return True

    def rpc_dec_camera_position(self):
        self.logger.info("dec_camera_position")

        self.events_mp["dec_camera_position"].set()
        return True

    def rpc_inc_camera_position(self):
        self.logger.info("inc_camera_position")

        self.events_mp["inc_camera_position"].set()
        return True

    def rpc_set_focus_position(self, value):
        self.logger.info("set_focus_position %s" % value)

        with self.setup_mp["focus_position"].get_lock():
            self.setup_mp["focus_position"].value = value

        self.events_mp["set_focus_position"].set()

        return True

    def rpc_dec_focus_position(self):
        self.logger.info("dec_focus_position")

        self.events_mp["dec_focus_position"].set()
        return True

    def rpc_inc_focus_position(self):
        self.logger.info("inc_focus_position")

        self.events_mp["inc_focus_position"].set()
        return True

    def rpc_reset(self):
        self.logger.info("reset")

        self.events_mp["reset"].set()
        return True

def main():
    FiberControlServer()

if __name__ == '__main__':
    main()
