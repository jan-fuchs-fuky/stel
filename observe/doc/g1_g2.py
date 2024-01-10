#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import math

def calc_pix2arcsec(title, offset_arcsec, x1, x2, y1, y2, dec=False):

    x = x1 - x2
    y = y1 - y2
    pixels = math.sqrt(x**2 + y**2)

    pix2arcsec = offset_arcsec / pixels

    print('%s 1 pix = %.6f arcsec' % (title, pix2arcsec))

    angle = math.degrees(math.acos(abs(x / pixels)))

    if dec:
        angle = 90 - angle

    print("angle = ", angle)

    return pix2arcsec

def main():
    # binning 1x1
    dec_pix2arcsec = calc_pix2arcsec("G1 DEC", 100.0, 295.0, 176.0, 130.0, 41.0, dec=True)
    ra_pix2arcsec = calc_pix2arcsec("G1 RA", 100.0, 269, 174, 134, 250)
    print("DIFF %.6f\n" % abs(dec_pix2arcsec - ra_pix2arcsec))

    # binning 1x1
    dec_pix2arcsec = calc_pix2arcsec("G2 DEC", 100.0, 987, 1350, 776, 107, dec=True)
    ra_pix2arcsec = calc_pix2arcsec("G2 RA", 100.0, 1002, 664, 771, 1138)
    print("DIFF %.6f\n" % abs(dec_pix2arcsec - ra_pix2arcsec))

if __name__ == '__main__':
    main()
