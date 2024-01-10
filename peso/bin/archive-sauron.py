#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import os
import sys
import configparser
import xmlrpc.client
import traceback

from astropy.io import fits

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

#FIBER_CONTROL_CLIENT_CFG = "%s/../etc/fiber_control_client.cfg" % SCRIPT_PATH
FIBER_CONTROL_CLIENT_CFG = "/home/fuky/git/observe/service/etc/fiber_control_client.cfg"

class ArchiveFitsFile:

    COMP_IDX = 0
    FLAT_IDX = 1

    def __init__(self):
        #self.instrument = "oes"
        self.instrument = "ccd700"

        self.load_cfg()

        relays = [-1, -1]
        try:
            relays = self.get_relays()
        except:
            # TODO: zalogovat a poslat na e-mail
            traceback.print_exc()

        toptec_values = {
            "cameras_position": -1,
            "focus_position": -1,
            "esw1a": -1,
            "esw1b": -1,
            "esw2a": -1,
            "esw2b": -1,
            "g1": -1,
            "g2": -1,
            "voltage_120": -1,
            "voltage_33": -1,
            "voltage_50": -1,
        }

        try:
            toptec_values = self.get_toptec_values()
            for key in ["voltage_120", "voltage_33", "voltage_50"]:
                toptec_values[key] /= 10.0

            for key in toptec_values:
                if isinstance(toptec_values[key], bool):
                    toptec_values[key] = int(toptec_values[key])
        except:
            # TODO: zalogovat a poslat na e-mail
            traceback.print_exc()

        filename = "/home/fuky/_tmp/2m/e202111060015_comp.fit"
        #filename = "/home/fuky/_tmp/2m/e202111060044_flat.fit"
        #filename = "/home/fuky/_tmp/2m/e202111060027_object.fit"
        with fits.open(filename, mode="update") as hdulist:
            hdr = hdulist[0].header
            self.append_relays(hdr, relays)
            self.append_toptec(hdr, toptec_values)

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(FIBER_CONTROL_CLIENT_CFG)

        self.cfg = {
            "camera": {},
        }

        self.cfg["toptec"] = dict(rcp.items("toptec"))
        self.cfg["quido"] = dict(rcp.items("quido"))

        camera = dict(rcp.items("camera"))
        for item in camera:
            # photometric_, ccd700_, oes_
            if item.startswith("%s_" % self.instrument):
                key = item.split("_", maxsplit=1)[1]
                self.cfg["camera"][key] = int(camera[item])

    def append_relays(self, hdr, relays):
        hdr.append(("REL-COMP", relays[self.COMP_IDX], "comp lamp relay (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("REL-FLAT", relays[self.FLAT_IDX], "flat lamp relay (0: ON, 1: OFF, -1: unknown)"))


    # CCD700_object_1fiber = 400
    # CCD700_object_2fiber = 1050
    # CCD700_comp_1fiber = 12965
    # CCD700_comp_2fiber = 13615
    # CCD700_flat_1fiber = 14600
    # CCD700_flat_2fiber = 15250
    # CCD700_flat_both_fibers = 14930
    def get_fiberid_ccd700(self, imagetyp, cameras_position):
        ids = [1, 2, 12]
        fiberid = 0
        counter = 0
        for suffix in ["1fiber", "2fiber", "both_fibers"]:
            key = "%s_%s" % (imagetyp, suffix)
            if key in self.cfg["camera"] and abs(self.cfg["camera"][key] - cameras_position) <= 3:
                fiberid = ids[counter]
            counter += 1

        return fiberid

    # OES_object = 1700
    # OES_comp = 14300
    # OES_flat = 15600
    def get_fiberid_oes(self, imagetyp, cameras_position):
        fiberid = 0
        if imagetyp in self.cfg["camera"] and abs(self.cfg["camera"][imagetyp] - cameras_position) <= 3:
            fiberid = 3

        return fiberid

    #  0 - unknown
    #  1 - first CCD700
    #  2 - second CCD700
    #  3 - OES
    # 12 - both CCD700
    # tolerance = +-3 increments
    def get_fiberid(self, imagetyp, cameras_position):
        get_fiberid_callback = {
            "oes": self.get_fiberid_oes,
            "ccd700": self.get_fiberid_ccd700,
        }

        try:
            fiberid = get_fiberid_callback[self.instrument](imagetyp, cameras_position)
        except:
            fiberid = 0 # unknown
            # TODO: zalogovat
            traceback.print_exc()

        return fiberid

    def append_toptec(self, hdr, values):
        # TODO: rozpracovano
        hdr.append(("FIBERID", self.get_fiberid(hdr["IMAGETYP"], values["cameras_position"]), "fiber ID (0: unknown)"))

        hdr.append(("FIBERPOS", values["cameras_position"], "fiber position (-1: unknown)"))

        hdr.append(("TT-CPOS", values["cameras_position"], "cameras position (-1: unknown)"))
        hdr.append(("TT-FPOS", values["focus_position"], "focus position (-1: unknown)"))
        hdr.append(("TT-ESW1A", values["esw1a"], "end switch 1a (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("TT-ESW1B", values["esw1b"], "end switch 1b (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("TT-ESW2A", values["esw2a"], "end switch 2a (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("TT-ESW2B", values["esw2b"], "end switch 2b (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("TT-G1", values["g1"], "G1 power (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("TT-G2", values["g2"], "G2 power (0: ON, 1: OFF, -1: unknown)"))
        hdr.append(("TT-V120", values["voltage_120"], "voltage 12V (-1: unknown)"))
        hdr.append(("TT-V33", values["voltage_33"], "voltage 3.3V (-1: unknown)"))
        hdr.append(("TT-V50", values["voltage_50"], "voltage 5V (-1: unknown)"))

    def get_relays(self):
        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["quido"])
        relays = proxy.quido_get_relays()

        return relays

    def get_toptec_values(self):
        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["toptec"])
        values = proxy.toptec_get_values()

        return values

def main():
    ArchiveFitsFile()

if __name__ == '__main__':
    main()
