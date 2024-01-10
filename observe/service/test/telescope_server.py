#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import xmlrpc.server

class TelescopeServer:

    def __init__(self):
        port = 9999

        self.coordinates = {
            "ra": "201747.20",
            "dec": "380158.55",
            "position": 0,
            "object": "P Cyg",
        }

        self.glst = {
            "state": 1,
            "dome": 0,
        }

        server = xmlrpc.server.SimpleXMLRPCServer(("127.0.0.1", port))
        server.register_introspection_functions()
        server.register_function(self.rpc_telescope_set_coordinates, "telescope_set_coordinates")
        server.register_function(self.rpc_telescope_execute, "telescope_execute")
        server.register_function(self.rpc_telescope_info, "telescope_info")

        server.serve_forever()

    def rpc_telescope_set_coordinates(self, params):
        print(params)

        for key, value in params.items():
            self.coordinates[key] = value

        return "1"

    def rpc_telescope_execute(self, cmd):
        print(cmd)

        if cmd == "TGRA 1":
            self.glst["state"] = 4 # tracking
            self.glst["dome"] = 1  # stop
        elif cmd == "TETR 0":
            self.glst["state"] = 3 # stop

        return "1"

    def rpc_telescope_info(self):

        info = {
            'DOPO': '216.50',
            'FOPO': '0.00',
            'GLST': '0 %(state)i 1 1 0 %(dome)i 4 4 4 33271 0 0 0 0' % self.glst,
            'OBJECT': self.coordinates["object"],
            'TRCS': '0',
            'TRGV': '-68.7 -183.9',
            'TRHD': '-28.6629 19.1082',
            'TRRD': '%(ra)s %(dec)s %(position)s' % self.coordinates,
            'TRUS': '0.0000 0.0000',
            'TSRA': '155845.81 034549.54 0',
            'UT': '2021-02-28 11:32:17',
        }

        return info

def main():
    TelescopeServer()

if __name__ == '__main__':
    main()
