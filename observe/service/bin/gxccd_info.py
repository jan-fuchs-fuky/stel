#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# This file is part of Observe (Observing System for Ondrejov).
#
# Observe is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Observe is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Observe.  If not, see <http://www.gnu.org/licenses/>.
#

import os
import sys

from _gxccd_cffi import ffi, lib

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

class CameraInfo:

    BOOLEAN_PARAMETERS = {
        "CONNECTED": lib.GBP_CONNECTED,
        "SUB_FRAME": lib.GBP_SUB_FRAME,
        "READ_MODES": lib.GBP_READ_MODES,
        "SHUTTER": lib.GBP_SHUTTER,
        "COOLER": lib.GBP_COOLER,
        "FAN": lib.GBP_FAN,
        "FILTERS": lib.GBP_FILTERS,
        "GUIDE": lib.GBP_GUIDE,
        "WINDOW_HEATING": lib.GBP_WINDOW_HEATING,
        "PREFLASH": lib.GBP_PREFLASH,
        "ASYMMETRIC_BINNING": lib.GBP_ASYMMETRIC_BINNING,
        "MICROMETER_FILTER_OFFSETS": lib.GBP_MICROMETER_FILTER_OFFSETS,
        "POWER_UTILIZATION": lib.GBP_POWER_UTILIZATION,
        "GAIN": lib.GBP_GAIN,
        "CONFIGURED": lib.GBP_CONFIGURED,
        "RGB": lib.GBP_RGB,
        "CMY": lib.GBP_CMY,
        "CMYG": lib.GBP_CMYG,
        "DEBAYER_X_ODD": lib.GBP_DEBAYER_X_ODD,
        "DEBAYER_Y_ODD": lib.GBP_DEBAYER_Y_ODD,
        "INTERLACED": lib.GBP_INTERLACED,
    }

    INTEGER_PARAMETERS = {
        "CAMERA_ID": lib.GIP_CAMERA_ID,
        "CHIP_W": lib.GIP_CHIP_W,
        "CHIP_D": lib.GIP_CHIP_D,
        "PIXEL_W": lib.GIP_PIXEL_W,
        "PIXEL_D": lib.GIP_PIXEL_D,
        "MAX_BINNING_X": lib.GIP_MAX_BINNING_X,
        "MAX_BINNING_Y": lib.GIP_MAX_BINNING_Y,
        "READ_MODES": lib.GIP_READ_MODES,
        "FILTERS": lib.GIP_FILTERS,
        "MINIMAL_EXPOSURE": lib.GIP_MINIMAL_EXPOSURE,
        "MAXIMAL_EXPOSURE": lib.GIP_MAXIMAL_EXPOSURE,
        "MAXIMAL_MOVE_TIME": lib.GIP_MAXIMAL_MOVE_TIME,
        "DEFAULT_READ_MODE": lib.GIP_DEFAULT_READ_MODE,
        "PREVIEW_READ_MODE": lib.GIP_PREVIEW_READ_MODE,
        "MAX_WINDOW_HEATING": lib.GIP_MAX_WINDOW_HEATING,
        "MAX_FAN": lib.GIP_MAX_FAN,
        "MAX_GAIN": lib.GIP_MAX_GAIN,
        "FIRMWARE_MAJOR": lib.GIP_FIRMWARE_MAJOR,
        "FIRMWARE_MINOR": lib.GIP_FIRMWARE_MINOR,
        "FIRMWARE_BUILD": lib.GIP_FIRMWARE_BUILD,
        "DRIVER_MAJOR": lib.GIP_DRIVER_MAJOR,
        "DRIVER_MINOR": lib.GIP_DRIVER_MINOR,
        "DRIVER_BUILD": lib.GIP_DRIVER_BUILD,
        "FLASH_MAJOR": lib.GIP_FLASH_MAJOR,
        "FLASH_MINOR": lib.GIP_FLASH_MINOR,
        "FLASH_BUILD": lib.GIP_FLASH_BUILD,
    }

    STRING_PARAMETERS = {
        "CAMERA_DESCRIPTION": lib.GSP_CAMERA_DESCRIPTION,
        "MANUFACTURER": lib.GSP_MANUFACTURER,
        "CAMERA_SERIAL": lib.GSP_CAMERA_SERIAL,
        "CHIP_DESCRIPTION": lib.GSP_CHIP_DESCRIPTION,
    }

    VALUES = {
        "CHIP_TEMPERATURE": lib.GV_CHIP_TEMPERATURE,
        "HOT_TEMPERATURE": lib.GV_HOT_TEMPERATURE,
        "CAMERA_TEMPERATURE": lib.GV_CAMERA_TEMPERATURE,
        "ENVIRONMENT_TEMPERATURE": lib.GV_ENVIRONMENT_TEMPERATURE,
        "SUPPLY_VOLTAGE": lib.GV_SUPPLY_VOLTAGE,
        "POWER_UTILIZATION": lib.GV_POWER_UTILIZATION,
        "ADC_GAIN": lib.GV_ADC_GAIN,
    }

    G1_VALUES_BLACKLIST = [
        lib.GV_HOT_TEMPERATURE,
        lib.GV_CAMERA_TEMPERATURE,
        lib.GV_ENVIRONMENT_TEMPERATURE,
        lib.GV_SUPPLY_VOLTAGE,
        lib.GV_POWER_UTILIZATION,
        lib.GV_ADC_GAIN,
    ]

    G2_IDS = [
        20008,
    ]

    def __init__(self):

        self.camera_id = None
        self.camera = lib.gxccd_initialize_usb(-1)
        print(self.camera)

        if self.camera == ffi.NULL:
            raise Exception("gxccd_initialize_usb() => NULL")

        self.integer_parameter = ffi.new("int *")
        self.boolean_parameter = ffi.new("bool *")
        self.float_value = ffi.new("float *")

        self.string_buffer_size = 1024
        self.string_buffer = ffi.new("char[%s]" % self.string_buffer_size)

        lib.gxccd_get_string_parameter(self.camera, lib.GSP_CAMERA_DESCRIPTION, self.string_buffer, self.string_buffer_size)
        print("Camera description: %s" % ffi.string(self.string_buffer).decode())

        self.chip_temp = ffi.new("float *")
        self.get_chip_temp()
        print("Camera chip temp: %.2f °C" % self.chip_temp[0])

        lib.gxccd_set_read_mode(self.camera, 0)

        self.get_all_boolean_parameters()
        self.get_all_integer_parameters() # set self.camera_id
        self.get_all_string_parameters()
        self.get_all_values() # use self.camera_id

    def __del__(self):
        lib.gxccd_release(self.camera)

    def get_all_boolean_parameters(self):
        for key in self.BOOLEAN_PARAMETERS:
            index = self.BOOLEAN_PARAMETERS[key]
            if lib.gxccd_get_boolean_parameter(self.camera, index, self.boolean_parameter) == -1:
                self.load_last_error("gxccd_get_boolean_parameter(index=%i)" % index)
                continue

            print("%s = %i" % (key, self.boolean_parameter[0]))

    def get_all_integer_parameters(self):
        for key in self.INTEGER_PARAMETERS:
            index = self.INTEGER_PARAMETERS[key]
            if lib.gxccd_get_integer_parameter(self.camera, index, self.integer_parameter) == -1:
                self.load_last_error("gxccd_get_integer_parameter(index=%i)" % index)
                continue

            if key == "CAMERA_ID":
                self.camera_id = self.integer_parameter[0]

            print("%s = %i" % (key, self.integer_parameter[0]))

    def get_all_string_parameters(self):
        for key in self.STRING_PARAMETERS:
            index = self.STRING_PARAMETERS[key]
            if lib.gxccd_get_string_parameter(self.camera, index, self.string_buffer, self.string_buffer_size) == -1:
                self.load_last_error("gxccd_get_string_parameter(index=%i)" % index)
                continue

            print("%s = %s" % (key, ffi.string(self.string_buffer).decode()))

    def get_all_values(self):
        values = {}

        for key in self.VALUES:
            index = self.VALUES[key]

            if self.camera_id not in self.G2_IDS and index in self.G1_VALUES_BLACKLIST:
                continue

            if lib.gxccd_get_value(self.camera, index, self.float_value) == -1:
                self.load_last_error("gxccd_get_value(index=%i)" % index)
                values[key.lower()] = 9999
                continue

            print("%s = %.2f" % (key, self.float_value[0]))
            values[key.lower()] = self.float_value[0]

        return values

    def get_chip_temp(self):
        lib.gxccd_get_value(self.camera, lib.GV_CHIP_TEMPERATURE, self.chip_temp)

    def load_last_error(self, fce_description):
        lib.gxccd_get_last_error(self.camera, self.string_buffer, self.string_buffer_size)

        self.last_error = ffi.string(self.string_buffer).decode()
        self.last_error = "%s failed: %s" % (fce_description, self.last_error)

        print(self.last_error)

def main():
    CameraInfo()

if __name__ == '__main__':
    main()
