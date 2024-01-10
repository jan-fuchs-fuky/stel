#! /usr/bin/env python
# -*- coding: utf-8 -*-

#
# $ fits_highlight.py input.fit 2 250 150 >/tmp/output.png
#
# OpenCV vyzaduje existenci /dev/raw1394:
#
#     # ln -s /dev/null /dev/raw1394
#
#     # vim /etc/udev/rules.d/raw1394.rules
#     KERNEL=="raw1394", GROUP="video"
#

import sys
import numpy
import cv2

from astropy.io import fits
from subprocess import check_output

def main():
    if (len(sys.argv) != 5):
        print("Usage: %s INPUT_FITS ZOOM SATURATION HIGHLIGHT" % sys.argv[0])
        sys.exit()

    input_fits = sys.argv[1]
    zoom = float(sys.argv[2])
    saturation = 3 * [int(sys.argv[3])]
    highlight = 3 * [int(sys.argv[4])]

    hdulist = fits.open(input_fits)

    img = cv2.cvtColor(hdulist[0].data, cv2.COLOR_GRAY2BGR)
    height, width, channels = img.shape

    img[numpy.where((img > saturation).all(axis=2))] = [0, 0, 0xFFFF]
    img[numpy.where((img > highlight).all(axis=2))] = [0, 0xFFFF, 0]

    #img = (img / 256).astype(numpy.uint8)

    ratio = 1 / zoom
    img = cv2.resize(img, (int(width * ratio), int(height * ratio)))

    retval, data = cv2.imencode(".png", img)
    data.tofile(sys.stdout)

if __name__ == '__main__':
    main()
