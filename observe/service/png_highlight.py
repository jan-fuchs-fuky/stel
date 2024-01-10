#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# $ png_highlight.py input.fit 2 250 150 >/tmp/output.png
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

from subprocess import check_output

def main():
    if (len(sys.argv) != 5):
        print("Usage: %s INPUT_FITS ZOOM SATURATION HIGHLIGHT" % sys.argv[0])
        sys.exit()

    input_fits = sys.argv[1]
    zoom = sys.argv[2]
    saturation = 3 * [int(sys.argv[3])]
    highlight = 3 * [int(sys.argv[4])]

    cmd = ["/usr/bin/fitspng", "-s", zoom, "-o", "/dev/stdout", input_fits]

    img_str = check_output(cmd)

    data = numpy.frombuffer(img_str, numpy.uint8)
    img = cv2.imdecode(data, cv2.IMREAD_COLOR)

    img[numpy.where((img > saturation).all(axis=2))] = [0, 0, 255]
    img[numpy.where((img > highlight).all(axis=2))] = [0, 255, 0]

    retval, data = cv2.imencode(".png", img)

    # https://stackoverflow.com/questions/908331/how-to-write-binary-data-to-stdout-in-python-3
    # https://stackoverflow.com/questions/55889401/obtaining-file-position-failed-while-using-ndarray-tofile-and-numpy-fromfile
    #data.tofile(sys.stdout)

    sys.stdout.buffer.write(data.tobytes())

if __name__ == '__main__':
    main()
