#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from lxml import objectify, etree

class MakeSetup:
    def __init__(self, setup_dat):
        fo = open(setup_dat)
        lines = fo.readlines()
        fo.close()

        setup_elt = etree.Element("setup_exposure_list")

        for line in lines:
            line = line.strip()
            if (line.startswith("#")):
                continue

            item_elt = etree.SubElement(setup_elt, "setup_exposure")
            data = line.split()
            elts = ["name", "ga", "dm", "sf", "flat", "comp", "range", "ccd"]

            for i, elt_name in enumerate(elts):
                elt = etree.SubElement(item_elt, elt_name)
                elt.text = data[i]

        print(etree.tostring(setup_elt, pretty_print=True, encoding="utf-8", xml_declaration=True))

def main():
    if (len(sys.argv) != 2):
        print "Usage: %s SETUP_DAT" % sys.argv[0]
        sys.exit()
        
    MakeSetup(sys.argv[1])

if __name__ == '__main__':
    main()
