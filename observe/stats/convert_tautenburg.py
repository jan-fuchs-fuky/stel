#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys

def main():
    if (len(sys.argv) != 3):
        print("Usage: %s TAUTENBURG_DAT normal|debug" % sys.argv[0])
        sys.exit()

    filename = sys.argv[1]
    output_format = sys.argv[2]
    
    fo = open(filename, "r")
    lines = fo.readlines()
    fo.close()
    
    for line in lines:
        yy_mm, exptime, percent, maxtime = line.split()
        fixpercent = float(exptime) / (float(maxtime) / 100.0)
        if (output_format == "debug"):
            print("%s %s %s %s %.1f%%" % (yy_mm, exptime, percent, maxtime, fixpercent))
        else:
            # 2018-01-01 12:00:00 1.5% exptime_sum = 5.9, observe_time_sum = 395.0
            print("%s-01 12:00:00 %.1f%% exptime_sum = %s, observe_time_sum = %s" % (yy_mm, fixpercent, exptime, maxtime))

if __name__ == '__main__':
    main()
