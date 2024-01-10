#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2020 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import os
import sys
import matplotlib
import numpy as np

from astropy.io import fits

from astropy.visualization import MinMaxInterval, ZScaleInterval, ImageNormalize, \
    LinearStretch, LogStretch, PowerStretch, SqrtStretch, SquaredStretch, AsinhStretch, \
    SinhStretch, HistEqStretch

from skimage.util import img_as_ubyte

from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel
from PyQt5.QtGui import QPixmap, QImage

class ShowFitsUI(QMainWindow):

    def __init__(self, filename):
        super(ShowFitsUI, self).__init__()

        self.setWindowTitle(os.path.basename(sys.argv[0]))

        label = QLabel(self)

        fits2image = Fits2Image(filename)

        image = fits2image.normalize(fits2image.scale_interval["zscale"], fits2image.scale_stretch["linear"])

        height, width = image.shape
        bytes_per_line = width

        # QImage.Format_Grayscale16 - The image is stored using an 16-bit grayscale format. (added in Qt 5.13)
        qimage = QImage(image, width, height, bytes_per_line, QImage.Format_Grayscale8)
        pixmap = QPixmap(qimage)

        label.setPixmap(pixmap)

        self.setCentralWidget(label)
        self.resize(pixmap.width(), pixmap.height())
        self.show()

class Fits2Image:

    def __init__(self, filename):
        self.scale_interval = {
            "zscale": ZScaleInterval(),
            "min_max": MinMaxInterval(),
        }

        self.scale_stretch = {
            "linear": LinearStretch(),
            "log": LogStretch(),
            "sqrt": SqrtStretch(),
            "squared": SquaredStretch(),
            "asinh": AsinhStretch(),
            "sinh": SinhStretch(),
        }

        hdulist = fits.open(filename)

        image = hdulist[0].data
        self.image = img_as_ubyte(image)

    def normalize(self, scale_interval, scale_stretch):
        image_normalize = ImageNormalize(self.image, interval=scale_interval, stretch=scale_stretch, clip=True)
        result = img_as_ubyte(image_normalize(self.image))

        return result

def main():
    if len(sys.argv) != 2:
        print("Usage: %s FITS" % sys.argv[0])
        sys.exit()

    filename = sys.argv[1]

    if os.path.isfile(filename):
        app = QApplication([])
        show_fits_ui = ShowFitsUI(filename)

        return app.exec()

    dirname = filename

    for item in os.listdir(dirname):
        filename = os.path.join(dirname, item)
        if os.path.isfile(filename) and item.endswith(".fit"):
            fits2image = Fits2Image(filename)
            for interval in fits2image.scale_interval.keys():
                for stretch in fits2image.scale_stretch.keys():
                    image = fits2image.normalize(fits2image.scale_interval[interval], fits2image.scale_stretch[stretch])
                    output = "/tmp/out/%s_%s_%s.png" % (os.path.basename(filename[:-4]), interval, stretch)
                    matplotlib.image.imsave(output, image)

if __name__ == '__main__':
    main()
