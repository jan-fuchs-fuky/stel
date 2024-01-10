#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from lxml import objectify, etree

class MakeSetup:
    def __init__(self, setup_dat):
        fo = open(setup_dat)
        lines = fo.readlines()
        fo.close()

        root_elt = etree.Element("manualSetupExposure")

        for line in lines:
            line = line.strip()
            if (line.startswith("# coef_angle")):
                setup_elt = etree.SubElement(root_elt, "coefAngle")
                elts = ["a", "b", "c"]
            elif (line.startswith("# coef_short")):
                setup_elt = etree.SubElement(root_elt, "coefShort")
                elts = ["a", "b", "c"]
            elif (line.startswith("# coef_long")):
                setup_elt = etree.SubElement(root_elt, "coefLong")
                elts = ["a", "b", "c"]
            elif (line.startswith("# cw_max dm sf flat comp")):
                setup_elt = etree.SubElement(root_elt, "setup")
                elts = ["dichroicMirror", "spectralFilter", "flatExposeTime", "compExposeTime"]

            if (not line) or (line.startswith("#")):
                continue

            data = line.split()
            item_elt = etree.SubElement(setup_elt, "item")
            item_elt.attrib["centralWaveLengthMax"] = data[0]
            for i, elt_name in enumerate(elts):
                elt = etree.SubElement(item_elt, elt_name)
                elt.text = data[i+1]

        print(etree.tostring(root_elt, pretty_print=True, encoding="utf-8", xml_declaration=True))

def main():
    if (len(sys.argv) != 2):
        print "Usage: %s MANUAL_SETUP_EXPOSURE_DAT" % sys.argv[0]
        sys.exit()
        
    MakeSetup(sys.argv[1])

if __name__ == '__main__':
    main()
