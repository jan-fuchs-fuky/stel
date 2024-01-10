#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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
import io
import sys
import cv2
import time
import collections
import argparse
import configparser
import sdnotify
import logging
import multiprocessing
import ctypes
import xmlrpc.server
import xmlrpc.client
import numpy as np

from astropy.io import fits
from astropy.visualization import simple_norm
from datetime import datetime
from logging.handlers import TimedRotatingFileHandler
from skimage.util import img_as_ubyte

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

FIBER_POINTING_CFG = "%s/../etc/fiber_pointing_server.cfg" % SCRIPT_PATH
FIBER_POINTING_LOG = "%s/../log/fiber_pointing_server_%%s.log" % SCRIPT_PATH

def init_logger(logger, filename):
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - %(message)s")

    # DBG
    #formatter = logging.Formatter(
    #    ("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - "
    #     "%(filename)s:%(lineno)s - %(funcName)s() - %(message)s - "))

    fh = TimedRotatingFileHandler(filename, when='D', interval=1, backupCount=365)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)

    logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

class PointingProcess(multiprocessing.Process):

    def __init__(self, name, lock_mp, events_mp, setup_mp):
        self.name = name
        self.lock_mp = lock_mp
        self.events_mp = events_mp
        self.setup_mp = setup_mp
        super(PointingProcess, self).__init__(name=name)

        process_name = "main_%s" % name
        self.logger = logging.getLogger("fiber_pointing_%s" % process_name)
        init_logger(self.logger, FIBER_POINTING_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.fits_deque = collections.deque(25 * [None], maxlen=25)

    def loop(self):
        time.sleep(1)

    def run(self):
        sd_nofifier = sdnotify.SystemdNotifier()

        while (not self.events_mp["exit"].is_set()):
            sd_nofifier.notify("WATCHDOG=1")

            try:
                self.loop()
            except:
                self.logger.exception("loop() exception")
                time.sleep(15)

class FiberPointing:

    def __init__(self, camera_name):
        self.camera_name = camera_name
        self.load_cfg()

        process_name = "xmlrpc_%s" % camera_name
        self.logger = logging.getLogger("fiber_pointing_%s" % process_name)
        init_logger(self.logger, FIBER_POINTING_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.fits_deque = collections.deque(25 * [None], maxlen=25)
        self.last_fits_filename = False

        self.lock_mp = multiprocessing.Lock()

        self.events_mp = {
            "exit": multiprocessing.Event(),
        }

        self.setup_mp = {
            #"target_x": multiprocessing.Value(ctypes.c_uint16, self.cfg["setup"]["target_x"]),
            #"target_y": multiprocessing.Value(ctypes.c_uint16, self.cfg["setup"]["target_y"]),
        }

        pointing_process = PointingProcess(camera_name, self.lock_mp, self.events_mp, self.setup_mp)
        pointing_process.daemon = True
        pointing_process.start()

        server = xmlrpc.server.SimpleXMLRPCServer((self.cfg["camera"]["host"], self.cfg["camera"]["port"]))
        server.register_introspection_functions()
        server.register_function(self.rpc_put_fits, "fiber_pointing_put_fits")
        server.register_function(self.rpc_get_fits, "fiber_pointing_get_fits")
        server.register_function(self.rpc_get_jpg, "fiber_pointing_get_jpg")
        server.register_function(self.rpc_get_last_fits_filename, "fiber_pointing_get_last_fits_filename")

        sd_nofifier = sdnotify.SystemdNotifier()
        sd_nofifier.notify("READY=1")

        server.serve_forever()

        self.events_mp["exit"].set()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(FIBER_POINTING_CFG)

        self.cfg = {
            "camera": {},
        }

        camera_callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
            "data_dir": rcp.get,
        }
        self.run_cfg_callbacks("camera", camera_callbacks)

        print(self.cfg)

    def fits2jpg(self, fits_binary, quality, norm):
        compression_params = []
        compression_params.append(cv2.IMWRITE_JPEG_QUALITY)
        compression_params.append(quality)

        fits_fo = io.BytesIO(fits_binary.data)
        hdulist = fits.open(fits_fo)
        img = img_as_ubyte(hdulist[0].data)

        if norm == "sqrt_minmax":
            norm = simple_norm(img, stretch="sqrt")
            img = norm(img)
            img = cv2.normalize(img, None, 0, 255, cv2.NORM_MINMAX)

        _, img_encoded = cv2.imencode(".jpg", img, compression_params)

        return img_encoded.tostring()

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key]("%s_%s" % (self.camera_name, section), key)

    def rpc_get_last_fits_filename(self):
        return self.last_fits_filename

    def rpc_get_fits(self):
        fits_binary = self.fits_deque[-1]

        if (fits_binary is None):
            return False

        return fits_binary

    def rpc_get_jpg(self, quality, norm):
        fits_binary = self.fits_deque[-1]

        if (fits_binary is None):
            return False

        if quality < 10 or quality > 100:
            quality = 95

        jpg_binary = self.fits2jpg(fits_binary, quality, norm)

        return jpg_binary

    def rpc_put_fits(self, fits_binary):
        self.logger.info("rcp_put_fits %s" % repr(fits_binary))

        self.fits_deque.append(fits_binary)

        fits_fo = io.BytesIO(fits_binary.data)
        hdulist = fits.open(fits_fo)
        prihdr = hdulist[0].header
        self.last_fits_filename = prihdr["FILENAME"]
        filename = os.path.join(self.cfg["camera"]["data_dir"], prihdr["FILENAME"])

        self.logger.info("write %s" % filename)

        fo = open(filename, "wb")
        fo.write(fits_binary.data)
        fo.close()

        return True

def main():

    epilog = (
        "Example1 (process pointing camera):\n\n    %s -n pointing\n\n"
        "Example2 (process photometric camera):\n\n    %s -n photometric\n\n"
    )

    parser = argparse.ArgumentParser(
        description="Process FITS from cameras.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=epilog)

    parser.add_argument("-n", "--name", type=str, default=None, metavar="NAME",
                        choices={ "pointing", "photometric" },
                        help="Control NAME camera")

    args = parser.parse_args()

    FiberPointing(args.name)

if __name__ == '__main__':
    main()
