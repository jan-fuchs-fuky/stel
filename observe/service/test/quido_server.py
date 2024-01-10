#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import xmlrpc.server

class QuidoServer:

    def __init__(self):
        self.relays = [0, 0, 0, 0]

        port = 6001

        server = xmlrpc.server.SimpleXMLRPCServer(("127.0.0.1", port))
        server.register_introspection_functions()
        server.register_function(self.rpc_get_relays, "quido_get_relays")
        server.register_function(self.rpc_set_relay, "quido_set_relay")

        server.serve_forever()

    def rpc_get_relays(self):
        print("quido_get_relays", self.relays)

        return self.relays

    def rpc_set_relay(self, number, value):
        self.relays[number-1] = value

        print("quido_set_relay", number, value)

        return True

def main():
    QuidoServer()

if __name__ == '__main__':
    main()
