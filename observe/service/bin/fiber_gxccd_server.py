#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2022 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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
# G2:
#
#     over-scan enabled:
#
#         CHIP_W = 2252
#         CHIP_D = 1509
#
#     CONNECTED = 1
#     SUB_FRAME = 1
#     READ_MODES = 1
#     SHUTTER = 1
#     COOLER = 1
#     FAN = 0
#     FILTERS = 1
#     GUIDE = 0
#     WINDOW_HEATING = 1
#     PREFLASH = 1
#     ASYMMETRIC_BINNING = 1
#     MICROMETER_FILTER_OFFSETS = 0
#     POWER_UTILIZATION = 1
#     GAIN = 1
#     CMY = 0
#     CMYG = 0
#     INTERLACED = 0
#     CAMERA_ID = 20008
#     CHIP_W = 2184
#     CHIP_D = 1472
#     PIXEL_W = 6800
#     PIXEL_D = 6800
#     MAX_BINNING_X = 4
#     MAX_BINNING_Y = 4
#     READ_MODES = 2
#     FILTERS = 5
#     MINIMAL_EXPOSURE = 100000
#     DEFAULT_READ_MODE = 0
#     PREVIEW_READ_MODE = 1
#     MAX_WINDOW_HEATING = 1
#     FIRMWARE_MAJOR = 2
#     FIRMWARE_MINOR = 3
#     FIRMWARE_BUILD = 1
#     DRIVER_MAJOR = 0
#     DRIVER_MINOR = 4
#     DRIVER_BUILD = 1
#     FLASH_MAJOR = 2
#     FLASH_MINOR = 0
#     FLASH_BUILD = 0
#     CAMERA_DESCRIPTION = G2-3200 MkII
#     MANUFACTURER = Moravian Instruments
#     CAMERA_SERIAL = G2KF3202-02000
#     CHIP_DESCRIPTION = KAF-3200ME
#     CHIP_TEMPERATURE = -15.17
#     HOT_TEMPERATURE = 17.59
#     CAMERA_TEMPERATURE = 17.59
#     ENVIRONMENT_TEMPERATURE = 17.59
#     SUPPLY_VOLTAGE = 13.74
#     POWER_UTILIZATION = 0.47
#     ADC_GAIN = 0.80
#
#     gxccd_get_boolean_parameter(index=127) failed: Not implemented for this camera
#     gxccd_get_boolean_parameter(index=128) failed: Not implemented for this camera
#     gxccd_get_boolean_parameter(index=131) failed: Not implemented for this camera
#     gxccd_get_boolean_parameter(index=132) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=10) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=11) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=15) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=16) failed: Not implemented for this camera
#
# G1:
#
#     CONNECTED = 1
#     SUB_FRAME = 1
#     READ_MODES = 1
#     SHUTTER = 0
#     COOLER = 0
#     FAN = 1
#     FILTERS = 0
#     GUIDE = 1
#     WINDOW_HEATING = 0
#     PREFLASH = 0
#     ASYMMETRIC_BINNING = 0
#     MICROMETER_FILTER_OFFSETS = 0
#     POWER_UTILIZATION = 0
#     GAIN = 0
#     RGB = 0
#     CMY = 0
#     CMYG = 0
#     DEBAYER_X_ODD = 1
#     DEBAYER_Y_ODD = 0
#     INTERLACED = 0
#     CAMERA_ID = 3379
#     CHIP_W = 656
#     CHIP_D = 494
#     PIXEL_W = 7400
#     PIXEL_D = 7400
#     MAX_BINNING_X = 1
#     MAX_BINNING_Y = 1
#     READ_MODES = 2
#     FILTERS = 0
#     MINIMAL_EXPOSURE = 125
#     MAXIMAL_MOVE_TIME = 32767
#     DEFAULT_READ_MODE = 1
#     PREVIEW_READ_MODE = 0
#     MAX_FAN = 1
#     FIRMWARE_MAJOR = 2
#     FIRMWARE_MINOR = 9
#     FIRMWARE_BUILD = 0
#     DRIVER_MAJOR = 0
#     DRIVER_MINOR = 4
#     DRIVER_BUILD = 1
#     CAMERA_DESCRIPTION = G1-0300
#     MANUFACTURER = Moravian Instruments
#     CAMERA_SERIAL = G1SX00300-0337
#     CHIP_DESCRIPTION = ICX424AL
#     CHIP_TEMPERATURE = 20.66
#     SUPPLY_VOLTAGE = 14.72
#
#     gxccd_get_boolean_parameter(index=127) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=10) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=14) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=16) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=134) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=135) failed: Not implemented for this camera
#     gxccd_get_integer_parameter(index=136) failed: Not implemented for this camera
#     gxccd_get_value(index=1) failed: Invalid index
#     gxccd_get_value(index=2) failed: Invalid index
#     gxccd_get_value(index=3) failed: Invalid index
#     gxccd_get_value(index=11) failed: Invalid index
#     gxccd_get_value(index=20) failed: Invalid index
#

import os
import io
import sys
import time
import collections
import argparse
import configparser
import logging
import multiprocessing
import ctypes
import sdnotify
import xmlrpc.server
import xmlrpc.client
import numpy as np

from _gxccd_cffi import ffi, lib
from astropy.io import fits
from logging.handlers import RotatingFileHandler
from datetime import datetime, timedelta, timezone
from subprocess import call

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

if "ICE_TELESCOPE_PLATOSPEC" in os.environ:
    sys.path.append("%s/../python" % SCRIPT_PATH)
    import Ice
    import PlatoSpec

FIBER_POINTING_FITS_IDX_DIR = "%s/../fits_idx/" % SCRIPT_PATH
FIBER_POINTING_CFG = "%s/../etc/fiber_gxccd_server.cfg" % SCRIPT_PATH
FIBER_POINTING_LOG = "%s/../log/fiber_gxccd_server_%%s.log" % SCRIPT_PATH

FIBER_GXCCD_STATUS = {
    "starting": 0,
    "ready": 1,
    "exposing": 2,
    "reading": 3,
    "failed": 255,
}

def init_logger(logger, filename):
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - %(message)s")

    # DBG
    #formatter = logging.Formatter(
    #    ("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - "
    #     "%(filename)s:%(lineno)s - %(funcName)s() - %(message)s - "))

    fh = RotatingFileHandler(filename, maxBytes=10485760, backupCount=10)
    fh.setLevel(logging.INFO)
    #fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)

    logger.setLevel(logging.INFO)
    #logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

def get_toptec_state(cfg):
    proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % cfg["toptec"])

    values = proxy.toptec_get_values()

    return values

def get_quido_state(cfg):
    proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % cfg["quido"])

    values = proxy.quido_get_relays()

    return values

class CameraControl:

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
        2656,
        80027,
    ]

    def __init__(self, cfg, logger):
        self.cfg = cfg
        self.camera_id = cfg["camera"]["id"]
        self.width = cfg["camera"]["width"]
        self.height = cfg["camera"]["height"]
        self.binning = 1
        self.image_width = None
        self.image_height = None
        self.logger = logger

        self.last_error = ""

        self.camera = lib.gxccd_initialize_usb(self.camera_id)
        print(self.camera)

        if self.camera == ffi.NULL:
            raise Exception("gxccd_initialize_usb() => NULL")

        # vyuziva fce self.is_connected()
        self.connected = ffi.new("bool *")

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

        if self.camera_id in self.G2_IDS:
            lib.gxccd_set_read_mode(self.camera, 0)

        self.image_bytes = 0
        self.exptime = 0
        self.use_shutter = False
        self.filter = 0
        self.fan = 1
        self.image_type = ""
        self.target = ""
        self.observers = ""
        self.read_mode = 0
        self.last_expose_dt = datetime.now(tz=timezone.utc)

        self.set_temperature(-15.0)
        self.set_filter(self.filter)
        self.set_fan(self.fan)

        self.get_all_boolean_parameters()
        self.get_all_integer_parameters()
        self.get_all_string_parameters()
        self.get_all_values()

        self.read_modes_name = {}
        for index in range(100):
            result = lib.gxccd_enumerate_read_modes(self.camera, index, self.string_buffer, self.string_buffer_size)
            if result == -1:
                break
            # C4-16000EC: read mode 3 = "16-bit" lo-gain (ostatni rezimy uvozovky nemaji a my je zde take nechceme)
            read_mode_name = ffi.string(self.string_buffer).decode().replace('"', '')
            self.logger.info("read mode %i = %s" % (index, read_mode_name))
            self.read_modes_name[index] = read_mode_name

        self.init_ice_telescope()

    def __del__(self):
        lib.gxccd_release(self.camera)
        self.destroy_ice_telescope()

    def init_ice_telescope(self):
        if not self.cfg["telescope"]["enable"] or self.cfg["telescope"]["type"] != "ice":
            self.ice_telescope_communicator = None
            self.ice_telescope_proxy = None
            self.logger.info("ice_telescope is disabled")
            return

        self.ice_telescope_communicator = Ice.initialize(sys.argv)

        base = self.ice_telescope_communicator.stringToProxy("Telescope:default -h %(host)s -p %(port)i" % self.cfg["telescope"])
        self.ice_telescope_proxy = PlatoSpec.TelescopePrx.checkedCast(base)
        if not self.ice_telescope_proxy:
            raise RuntimeError("Invalid proxy")

        self.logger.info("ice_telescope_proxy is created")

    def destroy_ice_telescope(self):
        if self.ice_telescope_communicator is not None:
            self.ice_telescope_communicator.destroy()
            self.logger.info("ice_telescope_communicator is destroyed")

    def get_all_boolean_parameters(self):
        for key in self.BOOLEAN_PARAMETERS:
            index = self.BOOLEAN_PARAMETERS[key]
            if lib.gxccd_get_boolean_parameter(self.camera, index, self.boolean_parameter) == -1:
                self.load_last_error("gxccd_get_boolean_parameter(index=%i)" % index)
                continue

            self.logger.info("%s = %i" % (key, self.boolean_parameter[0]))

    def get_all_integer_parameters(self):
        for key in self.INTEGER_PARAMETERS:
            index = self.INTEGER_PARAMETERS[key]
            if lib.gxccd_get_integer_parameter(self.camera, index, self.integer_parameter) == -1:
                self.load_last_error("gxccd_get_integer_parameter(index=%i)" % index)
                continue

            self.logger.info("%s = %i" % (key, self.integer_parameter[0]))

    def get_all_string_parameters(self):
        for key in self.STRING_PARAMETERS:
            index = self.STRING_PARAMETERS[key]
            if lib.gxccd_get_string_parameter(self.camera, index, self.string_buffer, self.string_buffer_size) == -1:
                self.load_last_error("gxccd_get_string_parameter(index=%i)" % index)
                continue

            self.logger.info("%s = %s" % (key, ffi.string(self.string_buffer).decode()))

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

            self.logger.debug("%s = %.2f" % (key, self.float_value[0]))
            values[key.lower()] = self.float_value[0]

        return values

    def get_chip_temp(self):
        lib.gxccd_get_value(self.camera, lib.GV_CHIP_TEMPERATURE, self.chip_temp)

    def make_fits_filename(self, exposure_begin_dt):
        prefix = self.cfg["camera"]["prefix"]

        night_dt = exposure_begin_dt - timedelta(hours=12)
        actual_night = night_dt.strftime("%Y%m%d")

        if not os.path.isdir(FIBER_POINTING_FITS_IDX_DIR):
            os.mkdir(FIBER_POINTING_FITS_IDX_DIR)

        fits_idx = 0
        fits_idx_filename = os.path.join(FIBER_POINTING_FITS_IDX_DIR, "%s%s.txt" % (prefix, actual_night))

        if os.path.isfile(fits_idx_filename):
            with open(fits_idx_filename, "r") as fo:
                fits_idx = int(fo.read().strip())

        fits_idx += 1

        with open(fits_idx_filename, "w") as fo:
            fo.write("%i\n" % fits_idx)

        filename = "%s%s%06i.fit" % (prefix, actual_night, fits_idx)

        return filename

    def abort(self):
        result = lib.gxccd_abort_exposure(self.camera, False)

        if result == -1:
            self.load_last_error("gxccd_abort_exposure(download=False)")
            return False

        self.logger.debug("gxccd_abort_exposure(download=False)")
        return True

    def stop(self):
        result = lib.gxccd_abort_exposure(self.camera, True)

        if result == -1:
            self.load_last_error("gxccd_abort_exposure(download=True)")
            return False

        self.logger.debug("gxccd_abort_exposure(download=True)")
        return True

    def expose(self, exptime, image_type, target, observers, read_mode):
        self.last_expose_dt = datetime.now(tz=timezone.utc)

        self.image_type = image_type
        self.target = target
        self.observers = observers
        self.read_mode = read_mode

        # automatic FAN ON
        if self.fan == 0:
            result = self.set_fan(1)
            self.logger.info("Automatic FAN ON => %s" % result)

        if self.camera_id in self.G2_IDS:
            result = lib.gxccd_set_read_mode(self.camera, self.read_mode)
            if result == -1:
                self.load_last_error("gxccd_set_read_mode(read_mode=%i)" % self.read_mode)
                return False

        use_shutter = True
        if image_type in ["dark", "zero"]:
            use_shutter = False

        self.load_fits_header()
        self.get_chip_temp()

        self.image_width = self.width // self.binning
        self.image_height = self.height // self.binning

        image_size = self.image_width * self.image_height
        self.image_bytes = image_size * 2

        self.exptime = exptime
        self.use_shutter = use_shutter

        result = lib.gxccd_start_exposure(self.camera, exptime, use_shutter, 0, 0, self.image_width, self.image_height)

        if result == -1:
            self.load_last_error("gxccd_start_exposure(exptime=%i, use_shutter=%s, width=%i, height=%i)" % \
                (exptime, use_shutter, self.image_width, self.image_height))
            return False

        self.logger.debug("gxccd_start_exposure(exptime=%i, use_shutter=%s, width=%i, height=%i) success" % \
                (exptime, use_shutter, self.image_width, self.image_height))

        return True

    def set_filter(self, value):
        if self.camera_id not in self.G2_IDS:
            return True

        result = lib.gxccd_set_filter(self.camera, value)

        if result == -1:
            self.load_last_error("gxccd_set_filter(index=%i)" % value)
            return False

        self.logger.debug("gxccd_set_filter(index=%i) success" % value)
        self.filter = value
        return True

    def set_fan(self, value):
        if self.camera_id in self.G2_IDS:
            return True

        result = lib.gxccd_set_fan(self.camera, value)

        if result == -1:
            self.load_last_error("gxccd_set_fan(speed=%i)" % value)
            return False

        self.logger.debug("gxccd_set_fan(speed=%i) success" % value)
        self.fan = value
        return True

    def set_temperature(self, value):
        if self.camera_id not in self.G2_IDS:
            return True

        result = lib.gxccd_set_temperature(self.camera, value)

        if result == -1:
            self.load_last_error("gxccd_set_temperature(temp=%f)" % value)
            return False

        self.logger.debug("gxccd_set_temperature(temp=%f) success" % value)

        return True

    def set_binning(self, x, y):
        if self.camera_id not in self.G2_IDS:
            return True

        result = lib.gxccd_set_binning(self.camera, x, y)

        if result == -1:
            self.load_last_error("gxccd_set_binning(x=%i, y=%i)" % (x, y))
            return False

        self.logger.debug("gxccd_set_binning(x=%i, y=%i) success" % (x, y))
        self.binning = x

        return True

    def set_preflash(self, preflash_time, clear_num):
        if self.camera_id not in self.G2_IDS:
            return True

        result = lib.gxccd_set_preflash(self.camera, preflash_time, clear_num)

        if result == -1:
            self.load_last_error("gxccd_set_preflash(preflash_time=%f, clear_num=%i)" % (preflash_time, clear_num))
            return False

        self.logger.debug("gxccd_set_preflash(preflash_time=%f, clear_num=%i) success" % (preflash_time, clear_num))

        return True

    def is_connected(self):
        result = lib.gxccd_get_boolean_parameter(self.camera, lib.GBP_CONNECTED, self.connected)

        if result == -1:
            self.load_last_error("gxccd_get_boolean_parameter(GBP_CONNECTED)")
            return False

        self.logger.debug("gxccd_get_boolean_parameter(GBP_CONNECTED) success => %s" % self.connected[0])

        if self.connected[0]:
            return True

        return False

    def load_last_error(self, fce_description):
        lib.gxccd_get_last_error(self.camera, self.string_buffer, self.string_buffer_size)

        self.last_error = ffi.string(self.string_buffer).decode()
        self.last_error = "%s failed: %s" % (fce_description, self.last_error)

        self.logger.error(self.last_error)

    def get_last_error(self):
        return self.last_error

    def hour_minute_second2seconds(self, dt):
        seconds = dt.hour * 3600 + dt.minute * 60 + dt.second

        return seconds

    def get_fits(self, exposure_begin_dt, exposure_end_dt):
        image = np.zeros((self.image_height, self.image_width), dtype=np.uint16)
        cimage = ffi.from_buffer(image)

        result = lib.gxccd_read_image(self.camera, cimage, self.image_bytes)

        if result == -1:
            self.load_last_error("gxccd_read_image(size=%i)" % self.image_bytes)
            return None

        header = fits.Header()
        header["OBSERVER"] = (self.observers, "Observers")
        header["EXPTIME"] = (self.exptime, "Length of observation excluding pauses")
        header["DARKTIME"] = (self.exptime, "Length of observation including pauses")
        header["TM_START"] = (self.hour_minute_second2seconds(exposure_begin_dt), exposure_begin_dt.strftime("%H:%M:%S, %s"))
        header["TM_END"] = (self.hour_minute_second2seconds(exposure_end_dt), exposure_end_dt.strftime("%H:%M:%S, %s"))
        header["UT"] = (exposure_begin_dt.strftime("%H:%M:%S"), "UTC of  start of observation")
        header["DATE-OBS"] = (exposure_begin_dt.strftime("%Y-%m-%d"), "UTC date start of observation")
        header["FILTER"] = (self.filter, "")
        header["IMAGETYP"] = (self.image_type, "")
        header["OBJECT"] = (self.target, "")
        header["ST"] = ("", "Local sidereal time at start of observation")
        header["SYSVER"] = (self.cfg["header"]["SYSVER"], self.cfg["header_comment"]["SYSVER"])
        header["FILENAME"] = (self.make_fits_filename(exposure_begin_dt), "")
        header["CCDTEMP"] = (self.chip_temp[0], "Detector temperature")

        if self.camera_id in self.G2_IDS:
            read_modes_name = "unknown"
            if self.read_mode in self.read_modes_name:
                read_mode_name = self.read_modes_name[self.read_mode]

            header["READMODE"] = (self.read_mode, read_mode_name)

        if self.camera_id not in self.G2_IDS:
            header["FAN"] = (self.fan, "Camera FAN (0 is off, 1 is on)")

        header["OBSERVAT"] = (self.cfg["header"]["OBSERVAT"], self.cfg["header_comment"]["OBSERVAT"])
        header["ORIGIN"] = ("PESO", "AsU AV CR Ondrejov")
        header["LATITUDE"] = (self.cfg["header"]["LATITUDE"], self.cfg["header_comment"]["LATITUDE"])
        header["LONGITUD"] = (self.cfg["header"]["LONGITUD"], self.cfg["header_comment"]["LONGITUD"])
        header["HEIGHT"] = (self.cfg["header"]["HEIGHT"], self.cfg["header_comment"]["HEIGHT"])
        header["TELESCOP"] = (self.cfg["header"]["TELESCOP"], self.cfg["header_comment"]["TELESCOP"])

        header["DETECTOR"] = (self.cfg["header"]["DETECTOR"], "Name of the detector")
        header["CHIPID"] = (self.cfg["header"]["CHIPID"], "Name of CCD chip")
        header["CCDXPIXE"] = (self.cfg["header"]["CCDXPIXE"], "Size in microns of the pixels, in X")
        header["CCDYPIXE"] = (self.cfg["header"]["CCDYPIXE"], "Size in microns of the pixels, in Y")
        header["CCDXSIZE"] = (self.image_width, "X Size in pixels of digitised frame")
        header["CCDYSIZE"] = (self.image_height, "Y Size in pixels of digitised frame")

        for state in [self.telescope_header, self.toptec_header]:
            for key in state:
                header[key] = (state[key], "")

        hdu = fits.PrimaryHDU(image, header=header)

        self.fits_bytesio = io.BytesIO()

        hdu.writeto(self.fits_bytesio, checksum=True)

        #print("GXX %s" % self.fits_bytesio.__sizeof__())

        self.fits_bytesio.seek(0)

        self.logger.debug("gxccd_read_image(size=%i) success" % self.image_bytes)
        return self.fits_bytesio

    def is_image_ready(self):
        ready = ffi.new("_Bool *", 0)
        result = lib.gxccd_image_ready(self.camera, ready)

        # TODO: osetrit
        if result == -1:
            self.load_last_error("gxccd_image_ready()")
            return False

        if ready[0]:
            return True

        self.logger.debug("gxccd_image_ready() success")
        return False

    def format_ra(self, ra):
        if len(ra) >= 8:
            ra = "%s:%s:%s" % (ra[:2], ra[2:4], ra[4:])

        return ra

    def format_dec(self, dec):
        if len(dec) >= 8:
            shift = 0
            if dec[0] in ['+', '-']:
                shift = 1
            dec = "%s:%s:%s" % (dec[:2+shift], dec[2+shift:4+shift], dec[4+shift:])

        return dec

    def get_telescope_header_ice(self):
        status = self.ice_telescope_proxy.get_status()

        ra = self.format_ra(status.coordinates.ra)
        dec = self.format_dec(status.coordinates.dec)

        telescope_header = {
            "RA": ra,
            "DEC": dec,
            "TELPOS": status.coordinates.position,
            "TELS1": status.speed1,
            "TELS2": status.speed2,
            "TELS3": status.speed3,
            "TELSCREW": status.dec_screw_limit,
            "TELDOME": status.dome_position,
            "TELFOCUS": status.focus_position,
            "TELCM": status.correction_model,
            "TELGST": status.global_state.telescope,
            "TELGSD": status.global_state.dome,
            "TELGSS": status.global_state.slit,
            "TELGSMC": status.global_state.mirror_cover,
            "TELGSF": status.global_state.focus,
            "TELSBITS": status.global_state.status_bits,
            "TELEBITS": status.global_state.error_bits,
            "TELUORA": status.user_offsets.ra,
            "TELUODEC": status.user_offsets.dec,
            "TELAORA": status.autoguider_offsets.ra,
            "TELAODEC": status.autoguider_offsets.dec,
            "TELUSRA": status.user_speeds.ra,
            "TELUSDEC": status.user_speeds.dec,
            "TELUSA": status.user_speeds.active,
            "TELHA": status.axes.ha,
            "TELDA": status.axes.da,
            "OCHUM": status.meteo_status.humidity,
            "OCRAIN": status.meteo_status.precipitation,
            "OCWINDD": status.meteo_status.wind_direction,
            "OCWINDS": status.meteo_status.wind_speed,
            "OCBRTE": status.meteo_status.brightness_east,
            "OCBRTN": status.meteo_status.brightness_north,
            "OCBRTW": status.meteo_status.brightness_west,
            "OCBRTS": status.meteo_status.brightness_south,
            "OCBRTM": status.meteo_status.brightness_max,
            "OCTEMP": status.meteo_status.temperature,
            "OCATM": status.meteo_status.atmospheric_pressure,
            "OCPGM": status.meteo_status.pyrgeometer,
        }

        return telescope_header

    def get_telescope_header_xmlrpc(self):
        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["telescope"])

        values = proxy.telescope_execute("TRRD")

        # 0123456789 0123456789
        # 095006.487 -390212.10 0
        ra, dec, position = values.split()

        ra = self.format_ra(ra)
        dec = self.format_dec(dec)

        telescope_header = {
            "RA": ra,
            "DEC": dec,
            # TODO
            #"position": position,
        }

        telescope_header["TLE-TRHD"] = proxy.telescope_execute("TRHD")
        telescope_header["TLE-TRGV"] = proxy.telescope_execute("TRGV")
        telescope_header["TLE-TRCS"] = proxy.telescope_execute("TRCS")
        telescope_header["TLE-TRUS"] = proxy.telescope_execute("TRUS")

        return telescope_header

    def get_telescope_header(self):
        if self.cfg["telescope"]["type"] == "ice":
           telescope_header = self.get_telescope_header_ice()
        else:
           telescope_header = self.get_telescope_header_xmlrpc()

        return telescope_header

    def load_fits_header(self):
        self.telescope_header = {}
        self.toptec_header = {}

        if self.cfg["telescope"]["enable"]:
            self.telescope_header = self.get_telescope_header()

        if self.cfg["toptec"]["enable"]:
            values = get_toptec_state(self.cfg)

            self.toptec_header = {
                "TELFOCUS": values["focus_position"],
                "CAMPOS": values["cameras_position"],
            }

class CameraProcess(multiprocessing.Process):

    def __init__(self, name, cfg, lock_mp, events_mp, setup_mp):
        self.name = name
        self.cfg = cfg
        self.lock_mp = lock_mp
        self.events_mp = events_mp
        self.setup_mp = setup_mp
        super(CameraProcess, self).__init__(name=name)

        process_name = "main_%i" % self.cfg["camera"]["id"]
        self.logger = logging.getLogger("fiber_camera_%s" % process_name)
        init_logger(self.logger, FIBER_POINTING_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.fits_deque = collections.deque(25 * [None], maxlen=25)

        self.events = {
            "start_exposure": self.start_exposure,
            "abort_exposure": self.abort_exposure,
            "stop_exposure": self.stop_exposure,
            "set_temperature": self.set_temperature,
            "set_filter": self.set_filter,
            "set_fan": self.set_fan,
            "set_binning": self.set_binning,
            "set_preflash": self.set_preflash,
        }

        self.camera_control = None
        self.exposure_begin_dt = None
        self.exposure_end_dt = None
        self.all_values_dt = datetime.utcnow() - timedelta(hours=24)
        self.init_exposure()

    def init_exposure(self, exptime=-1, exptime_pointing=-1, repeat=-1, delay_after_exposure=0,
            image_type="", target="", observers="", read_mode=0, abort=False, stop=False, scientific_exposure=False):
        self.exposure_time = exptime
        self.exposure_time_scientific = exptime
        self.exposure_time_pointing = exptime_pointing
        self.exposure_repeat = repeat
        self.delay_after_exposure = delay_after_exposure
        self.exposure_number = 0
        self.exposure_abort = abort
        self.exposure_stop = stop
        self.image_type = image_type
        self.target = target
        self.observers = observers
        self.read_mode = read_mode
        self.scientific_exposure = scientific_exposure

    def process_event(self, key):
        try:
            # TODO: osetrit chybu
            self.events[key]()
        except:
            self.logger.exception("Event '%s' exception" % key)

            with self.setup_mp["status"].get_lock():
                self.setup_mp["status"].value = FIBER_GXCCD_STATUS["failed"]
        finally:
            self.events_mp[key].clear()

        if (key == "abort_exposure"):
            self.exposure_abort = True
            with self.setup_mp["status"].get_lock():
                self.setup_mp["status"].value = FIBER_GXCCD_STATUS["ready"]
        elif (key == "stop_exposure"):
            # TODO: udelat presneji
            self.exposure_end_dt = datetime.utcnow()

            self.exposure_stop = True
            with self.setup_mp["status"].get_lock():
                self.setup_mp["status"].value = FIBER_GXCCD_STATUS["reading"]

    def finish_event(self):
        with self.setup_mp["status"].get_lock():
            status = self.setup_mp["status"].value

        if status == FIBER_GXCCD_STATUS["exposing"]:
            exposure_elapsed_time = (datetime.utcnow() - self.exposure_begin_dt).total_seconds()

            with self.setup_mp["exposure_elapsed_time"].get_lock():
                self.setup_mp["exposure_elapsed_time"].value = exposure_elapsed_time

            if (self.exposure_time > exposure_elapsed_time) and (not self.exposure_abort) and (not self.exposure_stop):
                time.sleep(0.05)
            else:
                # TODO: udelat presneji
                self.exposure_end_dt = datetime.utcnow()
                self.logger.debug("status = reading")
                with self.setup_mp["status"].get_lock():
                    self.setup_mp["status"].value = FIBER_GXCCD_STATUS["reading"]
        elif status == FIBER_GXCCD_STATUS["reading"]:
            if self.camera_control.is_image_ready():
                self.load_fits()
                self.logger.debug("status = ready")
                with self.setup_mp["status"].get_lock():
                    self.setup_mp["status"].value = FIBER_GXCCD_STATUS["ready"]

                if (self.exposure_number < self.exposure_repeat) and (not self.exposure_abort) and (not self.exposure_stop):
                    time.sleep(self.delay_after_exposure)
                    self.start_exposure(init=False)
            else:
                time.sleep(0.05)

    def get_all_values(self):
        dt = datetime.utcnow()
        diff = dt - self.all_values_dt

        if diff.total_seconds() < 5:
            return None

        self.all_values_dt = dt

        values = self.camera_control.get_all_values()

        self.lock_mp.acquire()
        try:
            for key in values:
                self.setup_mp[key].value = values[key]
        except:
            self.logger.exception("CameraProcess.get_all_values() exception")
        finally:
            self.lock_mp.release()

        return values

    def loop(self):
        for key in self.events_mp:
            if (self.events_mp[key].is_set()):
                if (key == "exit"):
                    return

                self.logger.info("Process event '%s'" % key)
                self.events_mp[key].clear()
                self.process_event(key)

        self.finish_event()
        values = self.get_all_values()

        # fce self.get_all_values() vraci hodnoty pouze 1x za 5 sekund
        if values is not None:
            chip_temperature = values["chip_temperature"]
            fan_off_temperature = self.cfg["camera"]["fan_on_temperature"] - 2
            diff = datetime.now(tz=timezone.utc) - self.camera_control.last_expose_dt

            # automatic FAN ON/OFF
            if self.camera_control.fan == 0 and chip_temperature > self.cfg["camera"]["fan_on_temperature"]:
                result = self.camera_control.set_fan(1)
                self.logger.info("Automatic FAN ON => %s (chip_temperature = %.2f)" % (result, chip_temperature))
            elif self.camera_control.fan == 1 and diff > timedelta(minutes=30) and chip_temperature < fan_off_temperature:
                result = self.camera_control.set_fan(0)
                self.logger.info("Automatic FAN OFF => %s (chip_temperature = %.2f)" % (result, chip_temperature))

    def load_fits(self):
        fits_bytesio = self.camera_control.get_fits(self.exposure_begin_dt, self.exposure_end_dt)

        self.logger.info("append fits_bytesio")
        #self.fits_deque.append(fits_bytesio)

        #print("PP %s" % fits_bytesio.__sizeof__())
        #print("R %s" % len(fits_bytesio.read(-1)))

        fits_binary = xmlrpc.client.Binary(fits_bytesio.read())
        res = self.proxy.fiber_pointing_put_fits(fits_binary)

        self.logger.info("%s" % res)
        self.logger.info("fits_binary %s" % len(fits_binary.data))

        #self.fits = self.fits_deque[0]

    def abort_exposure(self):
        if self.camera_control.abort():
            return True

        return False

    def stop_exposure(self):
        if self.camera_control.stop():
            return True

        return False

    def start_exposure(self, init=True):
        #values = {}
        #self.lock_mp.acquire()
        #for key in self.setup_mp:
        #    if (key.startswith("exposure_")):
        #        values[key] = self.setup_mp[key].value
        #self.lock_mp.release()

        if init:
            with self.setup_mp["exposure_time"].get_lock():
                exposure_time = self.setup_mp["exposure_time"].value

            with self.setup_mp["exposure_time_pointing"].get_lock():
                exposure_time_pointing = self.setup_mp["exposure_time_pointing"].value

            with self.setup_mp["exposure_repeat"].get_lock():
                exposure_repeat = self.setup_mp["exposure_repeat"].value

            with self.setup_mp["delay_after_exposure"].get_lock():
                delay_after_exposure = self.setup_mp["delay_after_exposure"].value

            with self.setup_mp["image_type"].get_lock():
                image_type = self.setup_mp["image_type"].value

            with self.setup_mp["target"].get_lock():
                target = self.setup_mp["target"].value

            with self.setup_mp["observers"].get_lock():
                observers = self.setup_mp["observers"].value

            with self.setup_mp["read_mode"].get_lock():
                read_mode = self.setup_mp["read_mode"].value

            self.next_filter = -1
            self.active_filters = []
            for idx in range(10):
                filter_key = "exposure_filter%i" % idx
                with self.setup_mp[filter_key].get_lock():
                    value = self.setup_mp[filter_key].value

                if value == 1:
                    self.next_filter = 0
                    self.active_filters.append(idx)

            image_type = image_type.decode("latin")
            target = target.decode("latin")
            observers = observers.decode("latin")

            self.init_exposure(exposure_time, exposure_time_pointing, exposure_repeat, delay_after_exposure, image_type, target, observers, read_mode)

        image_type = self.image_type
        self.exposure_time = self.exposure_time_pointing

        if self.name == "photometric":
            self.set_next_filter()

            if self.exposure_time_pointing == -1:
                self.scientific_exposure = True
                self.exposure_time = self.exposure_time_scientific
            else:
                if self.scientific_exposure:
                    self.exposure_time = self.exposure_time_pointing
                    image_type = "autoguider"
                else:
                    self.exposure_time = self.exposure_time_scientific

                self.scientific_exposure = not self.scientific_exposure

        with self.setup_mp["exposure_time"].get_lock():
            self.setup_mp["exposure_time"].value = self.exposure_time

        if not self.camera_control.expose(self.exposure_time, image_type, self.target, self.observers, self.read_mode):
            self.init_exposure()
            return False

        self.exposure_begin_dt = datetime.utcnow()

        with self.setup_mp["status"].get_lock():
            self.setup_mp["status"].value = FIBER_GXCCD_STATUS["exposing"]

        self.exposure_number += 1

        with self.setup_mp["exposure_number"].get_lock():
            self.setup_mp["exposure_number"].value = self.exposure_number

        return True

    def set_filter_run(self, index):
        with self.setup_mp["state_filter"].get_lock():
            self.setup_mp["state_filter"].value = -2

        result = self.camera_control.set_filter(index)

        if not result:
            index = -1

        with self.setup_mp["state_filter"].get_lock():
            self.setup_mp["state_filter"].value = index

        return result

    def set_filter(self):
        with self.setup_mp["filter"].get_lock():
            index = self.setup_mp["filter"].value

        result = self.set_filter_run(index)

        return result

    def set_next_filter(self):
        if self.next_filter == -1:
            return

        try:
            index = self.active_filters[self.next_filter]

            if self.set_filter_run(index):
                self.next_filter += 1

                if self.next_filter > (len(self.active_filters) - 1):
                    self.next_filter = 0
        except:
            self.logger.exception("CameraControl.next_filter() exception")

    def set_fan(self):
        with self.setup_mp["fan"].get_lock():
            value = self.setup_mp["fan"].value

        result = self.camera_control.set_fan(value)

        with self.setup_mp["fan"].get_lock():
            self.setup_mp["fan"].value = self.camera_control.fan

        return result

    def set_temperature(self):
        with self.setup_mp["temperature"].get_lock():
            temperature = self.setup_mp["temperature"].value

        result = self.camera_control.set_temperature(temperature)

        return result

    def set_binning(self):
        with self.setup_mp["binning_x"].get_lock():
            x = self.setup_mp["binning_x"].value

        with self.setup_mp["binning_y"].get_lock():
            y = self.setup_mp["binning_y"].value

        result = self.camera_control.set_binning(x, y)

        return result

    def set_preflash(self):
        with self.setup_mp["preflash_time"].get_lock():
            preflash_time = self.setup_mp["preflash_time"].value

        with self.setup_mp["preflash_clear_num"].get_lock():
            clear_num = self.setup_mp["preflash_clear_num"].value

        result = self.camera_control.set_preflash(preflash_time, clear_num)

        return result

    def get_power(self):

        # Chile C1
        if self.cfg["camera"]["id"] == 50084:
            return [True, False]
        elif self.cfg["quido"]["enable"]:
            relays = get_quido_state(self.cfg)
            power = True if relays[0] == 1 else False
            return [power, False]
        elif not self.cfg["toptec"]["enable"]:
            return [True, False]

        toptec_state = get_toptec_state(self.cfg)

        power_first = toptec_state["g1"]
        power_second = toptec_state["g2"]
        reattach = False

        # G1
        if self.cfg["camera"]["id"] == 3379:
            power = power_first
            if power:
                reattach = True
        # G2
        else:
            power = power_second
            if (not power_first) and power:
                reattach = True

        self.logger.debug("G1 POWER = %(g1)s, G2 POWER = %(g2)s" % toptec_state)

        return [power, reattach]

    def run(self):
        self.proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["storage"])

        sd_nofifier = sdnotify.SystemdNotifier()
        CONNECTED_COUNTER_START = 10
        connected_counter = CONNECTED_COUNTER_START
        delay = 0.01
        power = False
        reattach = False

        while (not self.events_mp["exit"].is_set()):
            sd_nofifier.notify("WATCHDOG=1")

            while (not self.events_mp["exit"].is_set()) and (self.camera_control is None):
                sd_nofifier.notify("WATCHDOG=1")

                try:
                    power, reattach = self.get_power()
                    if power:
                        self.camera_control = CameraControl(self.cfg, self.logger)
                        with self.setup_mp["status"].get_lock():
                            self.setup_mp["status"].value = FIBER_GXCCD_STATUS["ready"]
                    break
                except:
                    self.logger.exception("CameraControl.__init__() exception")
                    time.sleep(10)
                    if reattach:
                        try:
                            result = call(["sudo", "/opt/bin/usb_reattach"], timeout=10)
                            self.logger.info("call('sudo /opt/bin/usb_reattach') => %i" % result)
                        except:
                            self.logger.exception("call('sudo /opt/bin/usb_reattach') exception")

            if not power:
                time.sleep(5)
                continue

            try:
                if connected_counter <= 0:
                    connected_counter = CONNECTED_COUNTER_START
                    if not self.camera_control.is_connected():
                        raise Exception("Camera is not connected")

                self.loop()
                time.sleep(delay)
                connected_counter -= delay
            except:
                self.camera_control = None
                self.logger.exception("loop() exception")

                with self.setup_mp["status"].get_lock():
                    self.setup_mp["status"].value = FIBER_GXCCD_STATUS["failed"]

                time.sleep(15)

class FiberCamera:

    def __init__(self, camera_name):
        self.camera_name = camera_name

        self.load_cfg()

        process_name = "xmlrpc_%s" % camera_name
        self.logger = logging.getLogger("fiber_pointing_%s" % process_name)
        init_logger(self.logger, FIBER_POINTING_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.lock_mp = multiprocessing.Lock()

        self.events_mp = {
            "exit": multiprocessing.Event(),
            "start_exposure": multiprocessing.Event(),
            "abort_exposure": multiprocessing.Event(),
            "stop_exposure": multiprocessing.Event(),
            "set_temperature": multiprocessing.Event(),
            "set_filter": multiprocessing.Event(),
            "set_fan": multiprocessing.Event(),
            "set_binning": multiprocessing.Event(),
            "set_preflash": multiprocessing.Event(),
        }

        self.setup_mp = {
            "exposure_time": multiprocessing.Value(ctypes.c_double, 1),
            "exposure_time_pointing": multiprocessing.Value(ctypes.c_double, 1),
            "exposure_elapsed_time": multiprocessing.Value(ctypes.c_double, 0),
            "exposure_repeat": multiprocessing.Value(ctypes.c_uint32, 1),
            "exposure_number": multiprocessing.Value(ctypes.c_uint32, 0),
            "exposure_filter0": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter1": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter2": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter3": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter4": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter5": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter6": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter7": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter8": multiprocessing.Value(ctypes.c_uint8, 0),
            "exposure_filter9": multiprocessing.Value(ctypes.c_uint8, 0),
            "read_mode": multiprocessing.Value(ctypes.c_uint8, 0),
            "delay_after_exposure": multiprocessing.Value(ctypes.c_uint16, 1),
            "status": multiprocessing.Value(ctypes.c_uint8, FIBER_GXCCD_STATUS["starting"]),
            "temperature": multiprocessing.Value(ctypes.c_double, -15.0),
            "filter": multiprocessing.Value(ctypes.c_uint8, 0),
            "fan": multiprocessing.Value(ctypes.c_uint8, 0),
            "binning_x": multiprocessing.Value(ctypes.c_uint8, 1),
            "binning_y": multiprocessing.Value(ctypes.c_uint8, 1),
            "preflash_time": multiprocessing.Value(ctypes.c_double, 1.0),
            "preflash_clear_num": multiprocessing.Value(ctypes.c_uint8, 3),
            "image_type": multiprocessing.Array(ctypes.c_char, 64),
            "target": multiprocessing.Array(ctypes.c_char, 64),
            "observers": multiprocessing.Array(ctypes.c_char, 64),

            # -1 - unknown, -2 - moving
            "state_filter": multiprocessing.Value(ctypes.c_int8, -1),

            # CameraControl.VALUES: pristup k temto hodnotam je rizen pomoci self.lock_mp
            "chip_temperature": multiprocessing.Value(ctypes.c_double, 9999),
            "hot_temperature": multiprocessing.Value(ctypes.c_double, 9999),
            "camera_temperature": multiprocessing.Value(ctypes.c_double, 9999),
            "environment_temperature": multiprocessing.Value(ctypes.c_double, 9999),
            "supply_voltage": multiprocessing.Value(ctypes.c_double, 9999),
            "power_utilization": multiprocessing.Value(ctypes.c_double, 9999),
            "adc_gain": multiprocessing.Value(ctypes.c_double, 9999),
        }

        camera_process = CameraProcess(camera_name,
            self.cfg, self.lock_mp, self.events_mp, self.setup_mp)

        camera_process.daemon = True
        camera_process.start()

        server = xmlrpc.server.SimpleXMLRPCServer((self.cfg["camera"]["host"], self.cfg["camera"]["port"]))
        server.register_introspection_functions()
        server.register_function(self.rpc_start_exposure, "start_exposure")
        server.register_function(self.rpc_stop_exposure, "stop_exposure")
        server.register_function(self.rpc_get_status, "get_status")

        if camera_name == "pointing":
            server.register_function(self.rpc_set_fan, "set_fan")

        if camera_name == "photometric":
            server.register_function(self.rpc_set_temperature, "set_temperature")
            server.register_function(self.rpc_set_filter, "set_filter")
            server.register_function(self.rpc_set_binning, "set_binning")
            server.register_function(self.rpc_set_preflash, "set_preflash")

        sd_nofifier = sdnotify.SystemdNotifier()
        sd_nofifier.notify("READY=1")

        server.serve_forever()

        self.events_mp["exit"].set()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(FIBER_POINTING_CFG)

        self.cfg = {
            "storage": {},
            "camera": {},
            "header": {},
            "header_comment": {},
            "telescope": {},
            "toptec": {},
            "quido": {},
        }

        callbacks = {
            "enable": rcp.getboolean,
            "host": rcp.get,
            "port": rcp.getint,
            "type": rcp.get,
        }
        for section in ["telescope", "toptec", "quido"]:
            self.run_cfg_callbacks(section, callbacks, prefix=False)

        storage_callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
        }
        self.run_cfg_callbacks("storage", storage_callbacks)

        camera_callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
            "prefix": rcp.get,
            "id": rcp.getint,
            "width": rcp.getint,
            "height": rcp.getint,
            "fan_on_temperature": rcp.getfloat,
        }
        self.run_cfg_callbacks("camera", camera_callbacks)

        header_callbacks = {
            "DETECTOR": rcp.get,
            "CHIPID": rcp.get,
            "CCDXPIXE": rcp.getfloat,
            "CCDYPIXE": rcp.getfloat,
            "SYSVER": rcp.get,
            "OBSERVAT": rcp.get,
            "LATITUDE": rcp.getfloat,
            "LONGITUD": rcp.getfloat,
            "HEIGHT": rcp.getfloat,
            "TELESCOP": rcp.get,
        }
        self.run_cfg_callbacks("header", header_callbacks)

        header_comment_callbacks = {
            "SYSVER": rcp.get,
            "OBSERVAT": rcp.get,
            "LATITUDE": rcp.get,
            "LONGITUD": rcp.get,
            "HEIGHT": rcp.get,
            "TELESCOP": rcp.get,
        }
        self.run_cfg_callbacks("header_comment", header_comment_callbacks)

        print(self.cfg)

    def run_cfg_callbacks(self, section, callbacks, prefix=True):
        prefix_section = section

        if prefix:
            prefix_section = "%s_%s" % (self.camera_name, section)

        for key in callbacks:
            self.cfg[section][key] = callbacks[key](prefix_section, key)

    def rpc_set_filter(self, value):
        self.logger.info("set_filter(%i)" % value)

        with self.setup_mp["filter"].get_lock():
            self.setup_mp["filter"].value = value

        self.events_mp["set_filter"].set()

        return True

    def rpc_set_fan(self, value):
        self.logger.info("set_fan(%f)" % value)

        with self.setup_mp["fan"].get_lock():
            self.setup_mp["fan"].value = value

        self.events_mp["set_fan"].set()

        return True

    def rpc_set_temperature(self, value):
        self.logger.info("set_temperature(%f)" % value)

        with self.setup_mp["temperature"].get_lock():
            self.setup_mp["temperature"].value = value

        self.events_mp["set_temperature"].set()

        return True

    def rpc_set_binning(self, x, y):
        self.logger.info("set_binning(x=%i, y=%i)" % (x, y))

        with self.setup_mp["binning_x"].get_lock():
            self.setup_mp["binning_x"].value = x

        with self.setup_mp["binning_y"].get_lock():
            self.setup_mp["binning_y"].value = y

        self.events_mp["set_binning"].set()

        return True

    def rpc_set_preflash(self, preflash_time, clear_num):
        self.logger.info("set_preflash(preflash_time=%f, clear_num=%i)" % (preflash_time, clear_num))

        with self.setup_mp["preflash_time"].get_lock():
            self.setup_mp["preflash_time"].value = preflash_time

        with self.setup_mp["preflash_clear_num"].get_lock():
            self.setup_mp["preflash_clear_num"].value = clear_num

        self.events_mp["set_preflash"].set()

        return True

    def rpc_get_status(self):
        values = {}

        with self.setup_mp["exposure_elapsed_time"].get_lock():
            values["exposure_elapsed_time"] = self.setup_mp["exposure_elapsed_time"].value

        with self.setup_mp["exposure_time"].get_lock():
            values["exposure_time"] = self.setup_mp["exposure_time"].value

        with self.setup_mp["exposure_number"].get_lock():
            values["exposure_number"] = self.setup_mp["exposure_number"].value

        with self.setup_mp["status"].get_lock():
            values["status"] = self.setup_mp["status"].value

        with self.setup_mp["state_filter"].get_lock():
            values["filter"] = self.setup_mp["state_filter"].value

        self.lock_mp.acquire()
        try:
            for key in CameraControl.VALUES:
                key = key.lower()
                values[key] = self.setup_mp[key].value
        except:
            self.logger.exception("rpc_get_status() exception")
        finally:
            self.lock_mp.release()

        return values

    # TODO: odladit
    def rpc_stop_exposure(self):
        self.logger.info("stop_exposure()")

        self.events_mp["stop_exposure"].set()

        return True

    def rpc_start_exposure(self, exposure_time, exposure_time_pointing, exposure_repeat, delay_after_exposure, image_type, target, observers, read_mode, filters):
        self.logger.info("start_exposure(exposure_time=%i, exposure_time_pointing=%i, exposure_repeat=%i, delay_after_exposure=%i, image_type='%s', target='%s', observers='%s', read_mode=%i, filters=%s)" % \
            (exposure_time, exposure_time_pointing, exposure_repeat, delay_after_exposure, image_type, target, observers, read_mode, filters))

        with self.setup_mp["exposure_time"].get_lock():
            self.setup_mp["exposure_time"].value = exposure_time

        with self.setup_mp["exposure_time_pointing"].get_lock():
            self.setup_mp["exposure_time_pointing"].value = exposure_time_pointing

        with self.setup_mp["exposure_repeat"].get_lock():
            self.setup_mp["exposure_repeat"].value = exposure_repeat

        with self.setup_mp["delay_after_exposure"].get_lock():
            self.setup_mp["delay_after_exposure"].value = delay_after_exposure

        with self.setup_mp["image_type"].get_lock():
            self.setup_mp["image_type"].value = image_type.encode("latin")

        with self.setup_mp["target"].get_lock():
            self.setup_mp["target"].value = target.encode("latin")

        with self.setup_mp["observers"].get_lock():
            self.setup_mp["observers"].value = observers.encode("latin")

        with self.setup_mp["read_mode"].get_lock():
            self.setup_mp["read_mode"].value = read_mode

        for idx in range(10):
            value = 0
            if idx in filters:
                value = 1

            filter_key = "exposure_filter%i" % idx
            with self.setup_mp[filter_key].get_lock():
                self.setup_mp[filter_key].value = value

        self.events_mp["start_exposure"].set()

        return True

def main():

    epilog = (
        "Example1 (control pointing camera):\n\n    %s -t pointing\n\n"
        "Example2 (control photometric camera):\n\n    %s -t photometric\n\n"
    )

    parser = argparse.ArgumentParser(
        description="Control Moravian Instruments cameras.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=epilog)

    parser.add_argument("-n", "--name", type=str, default=None, metavar="NAME",
                        choices={ "pointing", "photometric" },
                        help="Control NAME camera")

    args = parser.parse_args()

    FiberCamera(args.name)

if __name__ == '__main__':
    main()
