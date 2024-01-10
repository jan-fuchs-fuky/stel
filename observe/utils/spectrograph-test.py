#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import xmlrpclib

set_cmds = [
    "SPCH 1",
    "SPCH 2",
    "SPCH 3",
    "SPRP 4",
    "SPAP 4",
    "SPRP 5",
    "SPAP 5",
    "SPCH 6",
    "SPCH 7",
    "SPCH 8",
    "SPCH 9",
    "SPCH 10",
    "SPCH 11",
    "SPCH 12",
    "SPCH 15",
    "SPCH 26",
    "SPCH 21",
    "SPCH 23",
    "SPRP 13",
    "SPAP 13",
    "SPRP 22",
    "SPAP 22",
]

proxy = xmlrpclib.ServerProxy("http://localhost:9999")

i = 0
for cmd in set_cmds:
    print proxy.spectrograph_execute("%s %i\n" % (cmd, i)).strip()
    i += 1

print proxy.spectrograph_info()
