#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import xmlrpc.server

class ToptecServer:

    def __init__(self):
        # CCD700_object_1fiber = 400
        # CCD700_object_2fiber = 1050
        # CCD700_comp_1fiber = 12965
        # CCD700_comp_2fiber = 13615
        # CCD700_flat_1fiber = 14600
        # CCD700_flat_2fiber = 15250
        # CCD700_flat_both_fibers = 14930
        # OES_object = 1700
        # OES_comp = 14300
        # OES_flat = 15600
        self.cameras_position = 13616
        self.focus_position = 4681

        port = 6000

        server = xmlrpc.server.SimpleXMLRPCServer(("127.0.0.1", port))
        server.register_introspection_functions()
        server.register_function(self.rpc_get_values, "toptec_get_values")
        server.register_function(self.rpc_set_camera_position, "toptec_set_camera_position")
        server.register_function(self.rpc_set_focus_position, "toptec_set_focus_position")

        server.serve_forever()

    def rpc_set_focus_position(self, value):
        self.focus_position = value

        print("toptec_set_focus_position", value)

        return True

    def rpc_set_camera_position(self, value):
        self.cameras_position = value

        print("toptec_set_camera_position", value)

        return True

    def rpc_get_values(self):

        values = {
            'cameras_position': self.cameras_position,
            'esw1a': False,
            'esw1b': False,
            'esw2a': False,
            'esw2b': False,
            'focus_position': self.focus_position,
            'g1': False,
            'g2': False,
            'voltage_120': 136,
            'voltage_33': 32,
            'voltage_50': 50,
        }

        return values

def main():
    ToptecServer()

if __name__ == '__main__':
    main()

