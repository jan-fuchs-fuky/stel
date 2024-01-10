#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# https://pyserial.readthedocs.io/en/latest/shortintro.html
#
# # apt install python3-serial
#

import os
import sys
import time
import serial

from ctypes import c_uint8
from datetime import datetime

class FiberControl:

    def __init__(self):

        cmds = {
            "C0": [ -1, [ 0x44 ] ],
            "C1": [ -1, [ 0x44 ] ],
            "C2": [ -1, [ 0x44 ] ],
            "C3": [ -1, [ 0x44 ] ],
            "C?": [ 2, [] ],
            "P1": [ 2, [] ],
            "P2": [ 2, [] ],
            "P7": [ 2, [] ],
            "P8": [ 2, [] ],
            "RR": [ -1, [ 0xE0 ] ],
            "SB": [ 1, [] ],
            "SA": [ 8, [] ],
            "S1+": [ -1, [ 0x44, 0x45 ] ],
            "S2+": [ -1, [ 0x44, 0x45 ] ],
            "S1-": [ -1, [ 0x44, 0x45 ] ],
            "S2-": [ -1, [ 0x44, 0x45 ] ],
            "V0": [ 1, [] ],
            "V1": [ 1, [] ],
            "V2": [ 1, [] ],
        }

        cmd = sys.argv[1]
        cmd_len = len(cmd)

        if (len(sys.argv) == 3):
            if (cmd not in [ "M1", "M2" ]):
                print("ERROR: Unkwnow cmd - %s" % sys.argv[1])
                return

            value = int(sys.argv[2])

            if (value < 0) or (value > 0xFFFF):
                print("ERROR: bad value - %i" % value)
                return

            move_cmds = {
                "M1": [ -1, [ 0x44, 0x45 ] ],
                "M2": [ -1, [ 0x44, 0x45 ] ],
            }

            cmds.update(move_cmds)

            byte1 = value >> 8
            byte2 = value & 0x00FF

            cmd_array_len = cmd_len + 2
            LongArray = c_uint8 * cmd_array_len
            cmd_array = LongArray()
            cmd_array[-2] = byte1
            cmd_array[-1] = byte2
        else:
            if (cmd not in cmds):
                print("ERROR: Unkwnow cmd - %s" % sys.argv[1])
                return

            cmd_array_len = cmd_len
            LongArray = c_uint8 * cmd_array_len
            cmd_array = LongArray()

        for idx in range(cmd_len):
            cmd_array[idx] = ord(cmd[idx])

        cmd_str = " ".join(list("%02X" % b for b in cmd_array))
        print("HEX CMD: %s" % cmd_str)

        result_size, expected_answer = cmds[cmd]

        self.hw_serial = serial.Serial("/dev/ttyS1", 9600, timeout=2)

        self.hw_serial.write(cmd_array)

        answer = self.get_answer(result_size, expected_answer)

        self.hw_serial.close()

        answer_str = " ".join(list("%02X" % b for b in answer))
        print("HEX RET: %s" % answer_str)

        self.answer2human(cmd, answer)

    def print_camera_status(self, status):
        if status == 0x30:
            print("G1: OFF, G2: OFF")
        elif status == 0x31:
            print("G1: ON, G2: OFF")
        elif status == 0x32:
            print("G1: OFF, G2: ON")
        elif status == 0x33:
            print("G1: ON, G2: ON")

    def two_bytes2int(self, bytes_array):
        number = -1

        if (len(bytes_array) == 2):
            byte1 = bytes_array[0] << 8
            byte2 = bytes_array[1]
            number = byte1 + byte2

        return number

    def print_axis_position(self, cmd, bytes_array):
        name = "Focus"
        if cmd == "P2":
            name = "Camera"

        print("%s position = %i" % (name, self.two_bytes2int(bytes_array)))

    def print_status(self, byte):
        status = {}
        status["esw1a"] = not bool(byte & 0b00000001)
        status["esw1b"] = not bool((byte >> 1) & 0b00000001)
        status["esw2a"] = not bool((byte >> 2) & 0b00000001)
        status["esw2b"] = not bool((byte >> 3) & 0b00000001)
        status["g1"] = bool((byte >> 4) & 0b00000001)
        status["g2"] = bool((byte >> 5) & 0b00000001)

        print(bin(byte))
        print(status)

    def print_voltage(self, value):
        voltage = value / 10.0
        print("%.02fV" % voltage)

    def answer2human(self, cmd, answer):
        if cmd == "C?":
            self.print_camera_status(answer[1])
        elif cmd == "SB":
            self.print_status(answer[0])
        elif cmd in ["P1", "P2"]:
            self.print_axis_position(cmd, answer)
        elif cmd in ["V0", "V1", "V2"]:
            self.print_voltage(answer[0])

    def get_answer(self, result_size, expected_answer=[]):
        result = []

        success = False
        for i in range(30):
            answer = self.hw_serial.read(size=1)
            if (not answer):
                continue

            for byte in answer:
                result.append(byte)

                if (byte in expected_answer):
                    success = True
                    break

            if (len(result) == result_size) or success:
                break

        #for byte in result:
        #    print("%X %s %i" % (byte, bin(byte), byte))

        return result

def main():
    if (len(sys.argv) not in [2, 3]):
        print(
"""
Usage: %s CMD

C0 - turns OFF all cameras
C1 - turns ON G1 and turns OFF G2
C2 - turns ON G2 and turns OFF G1
C3 - turns ON all cameras
C? - get

Px - get actual position of axis x (1/2)

RR - reset unit

SB - get status byte

SA - get status SB, V0, V1, V2, P1, P2

Sx+ - increment 1 step of axis x

Sx- - decrement 1 step of axis x

Vv - get power voltage

V0 - get 3.3V voltage

V1 - get 5V voltage

V2 - get 12V voltage

Mxyy

""" % sys.argv[0])

        sys.exit()

    FiberControl()

if __name__ == '__main__':
    main()
