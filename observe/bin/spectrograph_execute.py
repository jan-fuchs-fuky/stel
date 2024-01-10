#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import xmlrpclib

#
# Coude colimator:
#
#     open:   SPCH 3 1
#     closed: SPCH 3 2
#
# OES colimator:
#
#     open:   SPCH 21 1
#     closed: SPCH 21 2
#

def main():
    if (len(sys.argv) <= 1):
        print "Usage: %s 'ASCOL_CMD1 PARAM1 PARAM2' 'ASCOL_CMD2 PARAM1 PARAM2'" % sys.argv[0]
        sys.exit()
    
    proxy = xmlrpclib.ServerProxy("http://primula:8888")

    for cmd in sys.argv[1:]:
        print cmd
        result = proxy.spectrograph_execute(cmd).strip()
        print result

if __name__ == "__main__":
    main()
