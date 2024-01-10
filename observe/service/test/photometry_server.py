#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import xmlrpc.server

from datetime import datetime

FIBER_GXCCD_STATUS = {
    "starting": 0,
    "ready": 1,
    "exposing": 2,
    "reading": 3,
    "failed": 255,
}

class PhotometryServer:

    def __init__(self):
        port = 7001

        self.status = FIBER_GXCCD_STATUS["ready"]
        self.exposure_time = 0
        self.begin_dt = datetime.utcnow()

        server = xmlrpc.server.SimpleXMLRPCServer(("127.0.0.1", port))
        server.register_introspection_functions()
        server.register_function(self.rpc_set_filter, "set_filter")
        server.register_function(self.rpc_start_exposure, "start_exposure")
        server.register_function(self.rpc_get_status, "get_status")

        server.serve_forever()

    def rpc_set_filter(self, value):
        print("set_filter(%i)" % value)

        return True

    def rpc_get_status(self):

        if self.status == FIBER_GXCCD_STATUS["exposing"]:
            diff = (datetime.utcnow() - self.begin_dt).total_seconds()
            if diff > self.exposure_time:
                self.status = FIBER_GXCCD_STATUS["ready"]

        values = {}
        values["exposure_elapsed_time"] = 0
        values["exposure_time"] = 0
        values["exposure_number"] = 0
        values["status"] = self.status
        values["filter"] = 0

        print(values)
        return values

    def rpc_start_exposure(self, exposure_time, exposure_time_pointing, exposure_repeat, delay_after_exposure, image_type, target, observers):
        print("start_exposure(exposure_time=%i, exposure_time_pointing=%i, exposure_repeat=%i, delay_after_exposure=%i, image_type='%s', target='%s', observers='%s')" % \
            (exposure_time, exposure_time_pointing, exposure_repeat, delay_after_exposure, image_type, target, observers))

        self.status = FIBER_GXCCD_STATUS["exposing"]
        self.exposure_time = exposure_time
        self.begin_dt = datetime.utcnow()

        return True

def main():
    PhotometryServer()

if __name__ == '__main__':
    main()
