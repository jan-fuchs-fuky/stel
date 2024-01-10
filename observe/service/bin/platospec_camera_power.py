#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import xmlrpc.client

proxy = xmlrpc.client.ServerProxy("http://localhost:7007")

if len(sys.argv) != 2:
    print("Usage: %s on|off\n" % sys.argv[0])

    temperature = proxy.quido_get_temperature()
    relays = proxy.quido_get_relays()

    print("Actual state\n")
    print("    Temperature: %.2f\n" % temperature)

    idx = 1
    for value in relays:
        value = "on" if value == 1 else "off"
        print("    Relay %i = %s" % (idx, value))
        idx += 1

    print()
    sys.exit()

power_str = sys.argv[1]

power = 0
if power_str == "on":
    power = 1

proxy.quido_set_relay(1, power)
print("PlatoSpec camera power %s" % power_str)
