#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2023 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# This file is part of Observe (Observing System for Ondrejov).
#
# Observe is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Observe is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Observe.  If not, see <http://www.gnu.org/licenses/>.
#
# https://www.riverbankcomputing.com/static/Docs/PyQt5/
#
# https://doc.qt.io/qtforpython/search.html
# https://doc.qt.io/qt-5/qimage.html
# https://doc.qt.io/qt-5/qgraphicsitem.html#itemChange
# https://docs.astropy.org/en/stable/io/fits/
# https://scikit-image.org/docs/stable/api/skimage.util.html#skimage.util.img_as_ubyte
# https://docs.astropy.org/en/stable/visualization/normalization.html
# https://docs.astropy.org/en/stable/api/astropy.visualization.ZScaleInterval.html
#
# help(matplotlib.colors.Colormap)
#
# https://matplotlib.org/examples/user_interfaces/embedding_in_qt5.html
#
# FITS:
#
#    CROTA2, CDELT1, CDELT2, PC001001
#    http://tdc-www.harvard.edu/software/wcstools/cphead.wcs.html
#    https://danmoser.github.io/notes/gai_fits-imgs.html
#    http://hosting.astro.cornell.edu/~vassilis/isocont/node17.html
#
#    https://docs.astropy.org/en/stable/visualization/wcsaxes/
#
#        plt.imshow(hdu.data, vmin=-2.e-5, vmax=2.e-4, origin='lower')
#
#    https://docs.astropy.org/en/stable/wcs/index.html#wcslint
#
# https://docs.astropy.org/en/stable/wcs/
#
#     astropy.wcs contains utilities for managing World Coordinate System (WCS)
#     transformations in FITS files. These transformations map the pixel locations in
#     an image to their real-world units, such as their position on the sky sphere.
#     These transformations can work both forward (from pixel to sky) and backward
#     (from sky to pixel).
#
# FITS World Coordinate System (WCS)
# https://fits.gsfc.nasa.gov/fits_wcs.html
#
# https://docs.python.org/3/library/operator.html#operator.methodcaller
#

import matplotlib
matplotlib.use("Qt5Agg")
import os
import io
import cv2
import sys
import time
import math
import subprocess
import multiprocessing
import threading
import traceback
import collections
import configparser
import logging
import humanize
import xmlrpc.client
import numpy as np
import matplotlib.pyplot as plt

from operator import methodcaller
from datetime import datetime, timedelta

from matplotlib.figure import Figure
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg

from PyQt5 import uic
from PyQt5.QtGui import QPixmap, QPen, QImage, QBrush, QColor
from PyQt5.QtMultimedia import QSound

from PyQt5.QtCore import QRunnable, QThreadPool, QObject, pyqtSlot, pyqtSignal, \
    Qt, QRectF, QTimer

from PyQt5.QtWidgets import QApplication, QMainWindow, QGraphicsItem, QGraphicsScene, \
    QGraphicsPixmapItem, QWidget, QSizePolicy, QMessageBox, QVBoxLayout, QCheckBox

from logging.handlers import TimedRotatingFileHandler

from astropy.io import fits

from astropy.visualization import MinMaxInterval, ZScaleInterval, ImageNormalize, \
    LinearStretch, LogStretch, PowerStretch, SqrtStretch, SquaredStretch, AsinhStretch, \
    SinhStretch, HistEqStretch

from astropy.stats import gaussian_sigma_to_fwhm
from astropy.coordinates import Angle
from astropy import units

from skimage.util import img_as_ubyte
from skimage.transform import rotate

from photutils import centroid_com, centroid_1dg, centroid_2dg, fit_2dgaussian

from numpy.random import default_rng

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

FIBER_POINTING_CLIENT_UI = "%s/../share/fiber_pointing_client.ui" % SCRIPT_PATH
FIBER_POINTING_CLIENT_CFG = "%s/../etc/fiber_pointing_client.cfg" % SCRIPT_PATH
FIBER_POINTING_CLIENT_LOG = "%s/../log/fiber_pointing_client_%%s.log" % SCRIPT_PATH
FIBER_POINTING_CLIENT_SPLASH_SCREEN = "%s/../share/splash_screen.jpg" % SCRIPT_PATH
FIBER_POINTING_CLIENT_SOUNDS = "%s/../sounds" % SCRIPT_PATH

# TODO: sdilet kod s fiber_gxccd_server.py
FIBER_GXCCD_STATUS = {
    "starting": 0,
    "ready": 1,
    "exposing": 2,
    "reading": 3,
    "failed": 255,
}

def init_logger(logger, filename):
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - %(message)s")

    # DBG
    #formatter = logging.Formatter(
    #    ("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - "
    #     "%(filename)s:%(lineno)s - %(funcName)s() - %(message)s - "))

    fh = TimedRotatingFileHandler(filename, when='D', interval=1, backupCount=365)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)

    logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

class WorkerSignals(QObject):
    '''
    Defines the signals available from a running worker thread.

    Supported signals are:

    finished
        No data

    error
        `tuple` (exctype, value, traceback.format_exc() )

    result
        `object` data returned from processing, anything

    progress
        `int` indicating % progress

    '''
    finished = pyqtSignal()
    error = pyqtSignal(tuple)
    result = pyqtSignal(object)
    progress = pyqtSignal(dict)

class Worker(QRunnable):
    '''
    Worker thread

    Inherits from QRunnable to handler worker thread setup, signals and wrap-up.

    :param callback: The function callback to run on this worker thread. Supplied args and
                     kwargs will be passed through to the runner.
    :type callback: function
    :param args: Arguments to pass to the callback function
    :param kwargs: Keywords to pass to the callback function

    '''

    def __init__(self, fn, *args, **kwargs):
        super(Worker, self).__init__()

        # Store constructor arguments (re-used for processing)
        self.fn = fn
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()

        # Add the callback to our kwargs
        self.kwargs['progress_callback'] = self.signals.progress

    @pyqtSlot()
    def run(self):
        '''
        Initialise the runner function with passed args, kwargs.
        '''

        # Retrieve args/kwargs here; and fire processing using them
        try:
            result = self.fn(*self.args, **self.kwargs)
        except:
            traceback.print_exc()
            exctype, value = sys.exc_info()[:2]
            self.signals.error.emit((exctype, value, traceback.format_exc()))
        else:
            self.signals.result.emit(result)  # Return the result of the processing
        finally:
            self.signals.finished.emit()  # Done

class FlatsThread():

    def __init__(self, window):
        self.window = window

        self.ra = window.flats_ra_LE.text()
        self.dec = window.flats_dec_LE.text()

        self.offset_min = window.flats_offset_min_LE.text()
        self.offset_max = window.flats_offset_max_LE.text()

        self.adu_min = window.flats_adu_min_SB.value()
        self.adu_max = window.flats_adu_max_SB.value()

        self.sun_altitude = window.flats_sun_altitude_SB.value()

        self.exptime_min = window.flats_exptime_min_SB.value()
        self.exptime_max = window.flats_exptime_max_SB.value()

        self.ra = Angle("%s hours" % self.ra)
        self.dec = Angle("%s degrees" % self.dec)

        self.offset_min = Angle("%s degrees" % self.offset_min)
        self.offset_max = Angle("%s degrees" % self.offset_max)

        self.offset_min = int(self.offset_min.to(units.arcsec).value)
        self.offset_max = int(self.offset_max.to(units.arcsec).value)

        self.window.log("RA = %s, DEC = %s" % (self.ra, self.dec))
        self.window.log("offset_min = %i arcsec, offset_max = %i arcsec" % (self.offset_min, self.offset_max))

        self.rng = default_rng()

    def rng_sign(self):
        sign = 1
        number = self.rng.integers(0, 2)

        if number == 0:
            sign = -1

        return sign

    def loop(self, progress_callback):

        data = {
            "message": "started",
            "level": "info",
        }

        progress_callback.emit(data)

        while not self.window.flats_stop_event.is_set():
            # TODO: kontrolovat zda-li nove vygenerovane RA, DEC splnuji offset_min
            ra_sign = self.rng_sign()
            ra_offset = self.rng.integers(self.offset_min, self.offset_max) * ra_sign

            dec_sign = self.rng_sign()
            dec_offset = self.rng.integers(self.offset_min, self.offset_max) * dec_sign

            data["message"] = "%i %i" % (ra_offset, dec_offset)
            progress_callback.emit(data)
            time.sleep(1)

        return "success"

    def progress_fn(self, data):
        self.window.log("FLATS THREAD: %s" % data["message"], data["level"])

    def print_output(self, result):
        self.window.log("FLATS THREAD: %s" % result)

    def error_fn(self, error):
        exctype, value, format_exc = error

        #self.window.log("filter = %i => %s" % (idx, result))
        #self.window.log("filter = %i => %s" % (idx, result), "warning")
        self.window.log("FLATS THREAD: %s %s" % (exctype, value), "error")

    def thread_complete(self):
        self.window.log("FLATS THREAD: completed")

class MatplotlibCanvas(FigureCanvasQTAgg):
    """Ultimately, this is a QWidget (as well as a FigureCanvasAgg, etc.)."""

    def __init__(self, parent=None, width=4, height=4, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111)

        FigureCanvasQTAgg.__init__(self, fig)
        self.setParent(parent)

        FigureCanvasQTAgg.setSizePolicy(self,
                                   QSizePolicy.Expanding,
                                   QSizePolicy.Expanding)
        FigureCanvasQTAgg.updateGeometry(self)

class StarMatplotlibCanvas(MatplotlibCanvas):

    def plot_star(self, image, x, y):
        self.axes.cla()
        self.axes.imshow(image)
        self.axes.plot(x, y, color="#FF0000", marker='+', ms=50, mew=1)
        self.axes.set_ylim(0, 100)
        self.draw()

class SightGraphicsItem(QGraphicsItem):

    def __init__(self, width, height, color, x_SB=None, y_SB=None, rectangle=True):
        self.width = width
        self.height = height
        self.color = color
        self.x_SB = x_SB
        self.y_SB = y_SB
        self.rectangle = rectangle

        super(SightGraphicsItem, self).__init__()

        self.setFlag(QGraphicsItem.ItemSendsScenePositionChanges, True)

    def set_width_height(self, width, height):

        if (self.width != width) or (self.height != height):
            self.prepareGeometryChange()
            self.width = width
            self.height = height

    def paint(self, painter, option, widget):
        pen = QPen(self.color, 1, Qt.SolidLine)
        painter.setPen(pen)

        size = 25
        x_center = self.width / 2
        y_center = self.height / 2

        painter.drawLine(x_center-size, y_center, x_center+size, y_center)
        painter.drawLine(x_center, y_center-size, x_center, y_center+size)

        if self.rectangle:
            painter.drawRect(0, 0, self.width, self.height)

    def boundingRect(self):
        border_size = 10

        bleft = -border_size
        btop = -border_size
        bwidth = self.width + (border_size * 2)
        bheight = self.height + (border_size * 2)

        return QRectF(bleft, btop, bwidth, bheight)

    def mouseReleaseEvent(self, event):
        pos = event.pos()
        scene_pos = event.scenePos()

        x = scene_pos.x() - pos.x()
        y = scene_pos.y() - pos.y()

        print("mouseReleaseEvent", x, y, scene_pos.x(), scene_pos.y(), pos.x(), pos.y())

        if self.x_SB and self.y_SB:
            self.x_SB.setValue(x)
            self.y_SB.setValue(y)

        super(SightGraphicsItem, self).mouseReleaseEvent(event)

    def itemChange(self, change, value):

        if (change == QGraphicsItem.ItemPositionChange) and self.scene():
            rect = self.scene().sceneRect()

            # value is the new position.
            if not rect.contains(value):
                # Keep the item inside the scene rect.
                value.setX(min(rect.right(), max(value.x(), rect.left())))
                value.setY(min(rect.bottom(), max(value.y(), rect.top())))

        return super(SightGraphicsItem, self).itemChange(change, value)

class FiberPointingScene(QGraphicsScene):

    def __init__(self, parent=None):
        super(FiberPointingScene, self).__init__(parent)

#    def mousePressEvent(self, event):
#        point = event.scenePos()
#        print(point.x(), point.y())
#        super(FiberPointingScene, self).mousePressEvent(event)

class FiberPointingUI(QMainWindow):

    COLORS = {
        "info": 0x000000,
        "highlight": 0x0000FF,
        "warning": 0xFF9966,
        "error": 0xFF0000,
    }

    def __init__(self):
        super(FiberPointingUI, self).__init__()

        self.load_cfg()

        self.ds9_processes = []
        self.run_ds9(self.cfg["pointing_storage"])
        self.run_ds9(self.cfg["photometric_storage"])

        if self.ds9_processes:
            time.sleep(1)

        self.sounds = {}
        self.sounds["error"] = QSound(os.path.join(FIBER_POINTING_CLIENT_SOUNDS, "error.wav"))
        self.sounds["set"] = QSound(os.path.join(FIBER_POINTING_CLIENT_SOUNDS, "set.wav"))
        self.sounds["new_image"] = QSound(os.path.join(FIBER_POINTING_CLIENT_SOUNDS, "new_image.wav"))

        self.sounds_enabled = {}
        self.sounds_enabled["error"] = True
        self.sounds_enabled["set"] = True
        self.sounds_enabled["new_image"] = True

        #filename = "/home/fuky/stel/fotometrie/2019-10-12/2019-10-12_19-36-44_001.fit"
        #filename = "/home/fuky/stel/fotometrie/2019-09-21/2019-09-21_21-06-50_001.fit"
        filename = "/opt/camera_control_new/share/photometric.fit"
        self.image_orig = self.load_fits(filename)

        uic.loadUi(FIBER_POINTING_CLIENT_UI, self)

        self.logger = logging.getLogger("fiber_pointing_client")
        init_logger(self.logger, FIBER_POINTING_CLIENT_LOG % "gui")
        self.logger.info("Starting process 'fiber_pointing_client'")

        self.exit = threading.Event()
        self.stop_event = threading.Event()

        self.thread_logger = {}
        self.thread_exit = {}
        for key in ["telescope_command", "guider_read", "guider_command"]:
            self.thread_exit[key] = threading.Event()
            self.thread_exit[key].set()

            self.thread_logger[key] = logging.getLogger(key)
            init_logger(self.thread_logger[key], FIBER_POINTING_CLIENT_LOG % key)

        self.tabifyDockWidget(self.expose_dockWidget, self.focus_dockWidget)
        self.tabifyDockWidget(self.focus_dockWidget, self.flats_dockWidget)
        self.expose_dockWidget.raise_()

        self.expose_filter_layout = QVBoxLayout()
        self.expose_filter_GB.setLayout(self.expose_filter_layout)

        counter = 0
        self.expose_filter_checkboxes = {}
        for item in self.cfg["photometric_camera"]["filters"]:
            name = "%i %s" % (counter, item)
            self.expose_filter_checkboxes[name] = QCheckBox(name, self)
            counter += 1

        for key in self.expose_filter_checkboxes:
            self.expose_filter_layout.addWidget(self.expose_filter_checkboxes[key])

        # TODO: implementovat
        self.autoguider_min_star_brightness_LB.hide()
        self.autoguider_min_star_brightness_SB.hide()
        self.autoguider_max_star_brightness_LB.hide()
        self.autoguider_max_star_brightness_SB.hide()

        self.autoguider_on = False
        self.last_fits = ""
        self.last_fits_filename = None
        self.last_fits_imagetype = None
        self.camera_name = "photometric"

        self.star_matplotlib_canvas = StarMatplotlibCanvas(self.star_matplotlib_widget, width=5, height=4, dpi=100)
        self.star_matplotlib_layout.addWidget(self.star_matplotlib_canvas)

        self.autodetect_GI = SightGraphicsItem(100, 100, Qt.blue, rectangle=False)
        self.target_GI = SightGraphicsItem(100, 100, Qt.red, self.target_x_SB, self.target_y_SB)
        self.target_GI.setFlag(QGraphicsItem.ItemIsMovable, True)

        self.target_x_SB.valueChanged.connect(lambda: self.graphics_item_set(
            self.target_GI, self.target_x_SB.value, self.target_GI.setX))

        self.target_y_SB.valueChanged.connect(lambda: self.graphics_item_set(
            self.target_GI, self.target_y_SB.value, self.target_GI.setY))

        self.target_size_SB.valueChanged.connect(self.target_size_SB_valueChanged)

        self.source_GI = SightGraphicsItem(100, 100, Qt.green, self.source_x_SB, self.source_y_SB)
        self.source_GI.setFlag(QGraphicsItem.ItemIsMovable, True)

        self.source_x_SB.valueChanged.connect(lambda: self.graphics_item_set(
            self.source_GI, self.source_x_SB.value, self.source_GI.setX))

        self.source_y_SB.valueChanged.connect(lambda: self.graphics_item_set(
            self.source_GI, self.source_y_SB.value, self.source_GI.setY))

        self.source_autodetect_CB.toggled.connect(self.source_autodetect_toggled)

        self.inter_image_guiding_CHB.stateChanged.connect(self.inter_image_guiding_CHB_changed)
        self.expose_enable_preflash_CHB.stateChanged.connect(self.expose_enable_preflash_CHB_changed)

        self.actionAutoguider.triggered.connect(self.actionAutoguider_triggered)
        self.actionCamerasPower.triggered.connect(self.actionCamerasPower_triggered)

        self.actionMain_window.triggered.connect(lambda: self.view_show_window(self.actionMain_window, self.main_dockWidget))
        self.actionExpose_window.triggered.connect(lambda: self.view_show_window(self.actionExpose_window, self.expose_dockWidget))
        self.actionFocus_window.triggered.connect(lambda: self.view_show_window(self.actionFocus_window, self.focus_dockWidget))
        self.actionState_window.triggered.connect(lambda: self.view_show_window(self.actionState_window, self.state_dockWidget))
        self.actionLog_window.triggered.connect(lambda: self.view_show_window(self.actionLog_window, self.log_dockWidget))

        self.action_sounds_set.triggered.connect(lambda: self.set_sounds(self.action_sounds_set, "set"))
        self.action_sounds_new_image.triggered.connect(lambda: self.set_sounds(self.action_sounds_new_image, "new_image"))
        self.action_sounds_error.triggered.connect(lambda: self.set_sounds(self.action_sounds_error, "error"))

        self.flats_start_BT.clicked.connect(self.flats_start)
        self.flats_stop_BT.clicked.connect(self.flats_stop)

        self.telescope_up_BT.clicked.connect(lambda: self.telescope_move("up"))
        self.telescope_down_BT.clicked.connect(lambda: self.telescope_move("down"))
        self.telescope_right_BT.clicked.connect(lambda: self.telescope_move("right"))
        self.telescope_left_BT.clicked.connect(lambda: self.telescope_move("left"))

        self.expose_camera_CB.currentIndexChanged.connect(self.expose_camera_changed)
        #self.expose_camera_CB.setCurrentIndex(1)
        self.expose_camera_CB.setCurrentIndex(1)

        self.expose_binning_CB.currentIndexChanged.connect(self.expose_binning_changed)

        # editingFinished, valueChanged
        self.expose_ccd_temp_SB.valueChanged.connect(self.expose_ccd_temp_changed)
        self.expose_preflash_DSB.valueChanged.connect(self.expose_preflash_changed)
        self.expose_preflash_num_clear_SB.valueChanged.connect(self.expose_preflash_changed)

        self.scale_init()

        #zscale_interval = ZScaleInterval()
        #vmin, vmax = zscale_interval.get_limits(image)
        #norm = ImageNormalize(vmin=vmin, vmax=vmax)
        #plt.imshow(image, norm=norm)
        #plt.show()
        #zscale_image = zscale_interval(image)
        #image = img_as_ubyte(zscale_image)

        #if (image.dtype.name != "uint16"):
        #    raise Exception("Unknown image.dtype")

        pixmap = QPixmap(FIBER_POINTING_CLIENT_SPLASH_SCREEN)
        self.graphics_pixmap_item = QGraphicsPixmapItem(pixmap)

        self.scene = FiberPointingScene()
        self.scene.setSceneRect(0, 0, 1920, 1080)
        self.scene.addItem(self.graphics_pixmap_item)
        self.scene.addItem(self.target_GI)
        self.scene.addItem(self.source_GI)
        self.scene.addItem(self.autodetect_GI)

        self.graphicsView.setScene(self.scene)
        self.graphicsView.scale(1, -1)

        self.scale_previous = 1
        self.scale_DSB.valueChanged.connect(self.scale_change)

        self.move_source_to_target_BT.clicked.connect(self.move_source_to_target_clicked)
        self.expose_start_BT.clicked.connect(self.expose_start_clicked)
        self.expose_stop_BT.clicked.connect(self.expose_stop_clicked)

        self.actionAutoguider_triggered(False)

        self.timer = QTimer()
        self.timer.timeout.connect(self.loopLoadFits)
        self.timer.start(100)

        #self.expose_PB.setValue(30.5)
        #self.expose_PB.setFormat("remaining 00:10:00 (elapsed 00:00:10)")
        self.expose_PB.setFormat("ready")

        self.expose_remaining_LB.setText("00:10:00")
        self.expose_elapsed_LB.setText("00:00:40")

        self.proxy = {}

        # TODO: do separatniho procesu
        for item in ["pointing_camera", "photometric_camera"]:
            self.proxy[item] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg[item])

        # TODO: do separatniho procesu
        for item in ["pointing_storage", "photometric_storage"]:
            self.proxy[item] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg[item])

        self.resize(self.cfg["window"]["width"], self.cfg["window"]["height"])

        # TODO: rozpracovano BEGIN
        self.threadpool = QThreadPool()
        self.threadpool.setMaxThreadCount(10)

        self.stop_event.clear()
        self.thread_exit["photometric_camera_read"].clear()

        self.progress_callback_dict = {
            "telescope_command": self.progress_telescope_command_fn,
            "photometric_camera_read": self.progress_photometric_camera_read_fn,
            "photometric_camera_command": self.progress_photometric_camera_command_fn,
        }

        kwargs = {
            "name": "photometric_camera_read",
            "gui": self,
            "command": None,
            "thread_exit": self.thread_exit["photometric_camera_read"],
        }

        self.photometric_camera_read_thread = GuiderThread(self)
        photometric_camera_worker = Worker(self.photometric_camera_read_thread.run, **kwargs)

        self.threadpool.start(photometric_camera_worker)
        # END

        self.flats_thread = None
        self.flats_stop_event = threading.Event()

        self.play("set")
        self.show()

    def __del__(self):
        for process in self.ds9_processes:
            process.terminate()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(FIBER_POINTING_CLIENT_CFG)

        servers = [
            "pointing_camera",
            "pointing_storage",
            "photometric_camera",
            "photometric_storage",
        ]

        self.cfg = {
            "window": {},
        }

        for server in servers:
            self.cfg[server] = {}

            callbacks = {
                "host": rcp.get,
                "port": rcp.getint,
            }

            if server.endswith("_camera"):
                callbacks["read_modes"] = rcp.get
                callbacks["field_rotation_angle"] = rcp.getfloat
                callbacks["field_parity"] = rcp.getint
                callbacks["pix2arcsec"] = rcp.getfloat
                callbacks["target_x"] = rcp.getint
                callbacks["target_y"] = rcp.getint

            if server == "photometric_camera":
                callbacks["filters"] = rcp.get

            if server.endswith("_storage"):
                callbacks["data_dir"] = rcp.get
                callbacks["save"] = rcp.getboolean
                callbacks["ds9_dir"] = rcp.get
                callbacks["ds9_bin"] = rcp.get
                callbacks["ds9_title"] = rcp.get
                callbacks["xpaset_bin"] = rcp.get

            self.run_cfg_callbacks(server, callbacks)

            if server.endswith("_camera"):
                self.cfg[server]["read_modes"] = self.cfg[server]["read_modes"].split(",")

            if server == "photometric_camera":
                self.cfg[server]["filters"] = self.cfg[server]["filters"].split(",")

        window_callbacks = {
            "width": rcp.getint,
            "height": rcp.getint,
        }
        self.run_cfg_callbacks("window", window_callbacks)

        print(self.cfg)

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key](section, key)

    def fce_ds9_process(self, ds9_bin, title):
        cmd = [ds9_bin, "-title", title]

        subprocess.call(cmd)

    def run_ds9(self, storage_cfg):
        try:
            if storage_cfg["ds9_bin"]:
                process = multiprocessing.Process(target=self.fce_ds9_process, args=(storage_cfg["ds9_bin"], storage_cfg["ds9_title"]))
                process.start()

                self.ds9_processes.append(process)
        except:
            traceback.print_exc()

    def scale_init(self):
        self.scale_interval = MinMaxInterval()
        self.scale_stretch = LinearStretch()

        self.scale_interval_from_text = {
            "zscale": ZScaleInterval(),
            "min max": MinMaxInterval(),
        }

        self.scale_stretch_from_text = {
            "linear": LinearStretch(),
            "log": LogStretch(),
            #"power": PowerStretch(),
            "sqrt": SqrtStretch(),
            "squared": SquaredStretch(),
            "asinh": AsinhStretch(),
            "sinh": SinhStretch(),
            #"histogram": HistEqStretch(),
        }

        self.scale_interval_CB.currentIndexChanged.connect(self.scale_interval_set)
        self.scale_stretch_CB.currentIndexChanged.connect(self.scale_stretch_set)

    # /usr/bin/ds9 -title mi
    def ds9_xpaset(self, filename, storage_cfg):

        if storage_cfg["ds9_dir"]:
            filename = os.path.basename(filename)
            filename = os.path.join(storage_cfg["ds9_dir"], filename)

        cmd = [storage_cfg["xpaset_bin"], "-p", storage_cfg["ds9_title"], "file", filename]
        print(cmd)

        try:
            output = subprocess.check_output(cmd)
            print(output)
        except:
            traceback.print_exc()

    # TODO: refaktorizovat, pipnout pri hodnotach mimo rozsah
    def refresh_state(self, camera_name, status):
        status_str = self.status2human(status["status"])

        if camera_name == "photometric":
            self.g2_chip_temp_LB.setText("%.1f °C" % status["chip_temperature"])
            self.g2_supply_voltage_LB.setText("%.1f V" % status["supply_voltage"])
            self.g2_power_utilization_LB.setText("%.1f %%" % (status["power_utilization"] * 100))
            self.g2_state_LB.setText(status_str)
            self.g2_filter_LB.setText(self.filter2human(status["filter"]))
            # TODO: ostatni hodnoty ukladat pouze do logu (ne prilis casto)

            diff = self.expose_ccd_temp_SB.value() - status["chip_temperature"]
            if abs(diff) < 0.5:
                self.g2_chip_temp_LB.setStyleSheet("background-color: #CCFFCC;")
            else:
                self.g2_chip_temp_LB.setStyleSheet("background-color: #FFCCCC;")

            if status["power_utilization"] < 0.9:
                self.g2_power_utilization_LB.setStyleSheet("background-color: #CCFFCC;")
            else:
                self.g2_power_utilization_LB.setStyleSheet("background-color: #FFCCCC;")

            if 11.5 < status["supply_voltage"] < 15.5:
                self.g2_supply_voltage_LB.setStyleSheet("background-color: #CCFFCC;")
            else:
                self.g2_supply_voltage_LB.setStyleSheet("background-color: #FFCCCC;")

            if status["status"] >= 128:
                self.g2_state_LB.setStyleSheet("background-color: #FFCCCC;")
            else:
                self.g2_state_LB.setStyleSheet("background-color: #CCFFCC;")

            if status["filter"] < 0:
                self.g2_filter_LB.setStyleSheet("background-color: #FFCCCC;")
            else:
                self.g2_filter_LB.setStyleSheet("background-color: #CCFFCC;")
        else:
            self.g1_chip_temp_LB.setText("%.1f °C" % status["chip_temperature"])
            self.g1_state_LB.setText(status_str)

            if status["chip_temperature"] < 30.0:
                self.g1_chip_temp_LB.setStyleSheet("background-color: #CCFFCC;")
            else:
                self.g1_chip_temp_LB.setStyleSheet("background-color: #FFCCCC;")

            if status["status"] < 128:
                self.g1_state_LB.setStyleSheet("background-color: #CCFFCC;")
            else:
                self.g1_state_LB.setStyleSheet("background-color: #FFCCCC;")

    def filter2human(self, value):
        filter_str = "unknown"

        if value == -2:
            filter_str = "moving"
        elif value >= 0:
            filter_str = "%i" % value

        return filter_str

    def status2human(self, status):
        status_str = "unknown"
        for key in FIBER_GXCCD_STATUS:
            if (FIBER_GXCCD_STATUS[key] == status):
                status_str = key

        return status_str

    # TODO
    def loopLoadFits(self):
        storage_cfg = self.cfg["%s_storage" % self.camera_name]

        cameras_status = {}
        for camera in ["photometric", "pointing"]:
            cameras_status[camera] = self.proxy["%s_camera" % camera].get_status()

        exposure_time = cameras_status[self.camera_name]["exposure_time"]
        elapsed_time = cameras_status[self.camera_name]["exposure_elapsed_time"]
        remaining_time = exposure_time - elapsed_time
        percent = elapsed_time / (exposure_time / 100.0)

        status = self.status2human(cameras_status[self.camera_name]["status"])

        self.expose_PB.setFormat("%s (%i/%i)" % (status, cameras_status[self.camera_name]["exposure_number"], self.expose_count_repeat_SB.value()))

        if (status == "ready"):
            self.expose_remaining_LB.setText("0:00:00")
            self.expose_elapsed_LB.setText("0:00:00")
            self.expose_PB.setValue(0)
        else:
            self.expose_remaining_LB.setText(str(timedelta(seconds=round(remaining_time))))
            self.expose_elapsed_LB.setText(str(timedelta(seconds=round(elapsed_time))))
            self.expose_PB.setValue(percent)

        self.refresh_state("photometric", cameras_status["photometric"])
        self.refresh_state("pointing", cameras_status["pointing"])

        # DBG begin
        #if not self.last_fits:
        #    #self.last_fits = "/home/fuky/stel/pointing/2020-01-11/g202001110287.fit"
        #    self.last_fits = "/home/fuky/stel/fotometrie/2019-12-18/2019-12-18_19-03-26_001.fit"
        #    self.image_orig = self.load_fits(self.last_fits)
        #    self.reaload_pixmap()
        #    self.graphics_item_set(self.target_GI)
        #return
        # DBG end

        last_fits_filename = self.proxy["%s_storage" % self.camera_name].fiber_pointing_get_last_fits_filename()
        if (not last_fits_filename) or (self.last_fits_filename == last_fits_filename):
            return

        image_format = self.image_format_CB.currentText()

        if image_format.startswith("JPG"):
            quality = 95 # high
            if image_format.find("medium") != -1:
                quality = 70
            elif image_format.find("low") != -1:
                quality = 50

            jpg_binary = self.proxy["%s_storage" % self.camera_name].fiber_pointing_get_jpg(quality, "")

            if jpg_binary:
                self.last_fits_filename = last_fits_filename
                jpg_array = np.frombuffer(jpg_binary.data, np.uint8)
                self.image_orig = cv2.imdecode(jpg_array, cv2.IMREAD_GRAYSCALE)
                self.reaload_pixmap()
                self.graphics_item_set(self.target_GI)
                self.play("new_image")

                if storage_cfg["save"]:
                    filename = os.path.join(storage_cfg["data_dir"], last_fits_filename.replace(".fit", ".jpg"))
                    print(filename)
                    fo = open(filename, "wb")
                    fo.write(jpg_binary.data)
                    fo.close()

                self.image_size_LB.setText(humanize.naturalsize(jpg_array.nbytes))
        else:
            fits_binary = self.proxy["%s_storage" % self.camera_name].fiber_pointing_get_fits()

            if fits_binary:
                fits_fo = io.BytesIO(fits_binary.data)
                hdulist = fits.open(fits_fo)
                prihdr = hdulist[0].header
                self.last_fits_filename = prihdr["FILENAME"]
                self.last_fits_imagetype = prihdr["IMAGETYP"]
                filename = os.path.join(storage_cfg["data_dir"], prihdr["FILENAME"])

                self.image_orig = img_as_ubyte(hdulist[0].data)
                self.reaload_pixmap()
                self.graphics_item_set(self.target_GI)
                self.play("new_image")

                if storage_cfg["save"]:
                    print(filename)
                    fo = open(filename, "wb")
                    fo.write(fits_binary.data)
                    fo.close()

                if storage_cfg["ds9_bin"]:
                    self.ds9_xpaset(filename, storage_cfg)

                self.image_size_LB.setText("")

        # DBK
        return

        fits_dir = "/tmp/test"

        filenames = os.listdir(fits_dir)
        if (not filenames):
            return

        filenames.sort()
        last_fits = os.path.join(fits_dir, filenames[-1])

        if (last_fits != self.last_fits):
            self.last_fits = last_fits
            print("load %s" % last_fits)
            self.image_orig = self.load_fits(last_fits)
            self.reaload_pixmap()

            # TODO: vyresit poradne
            self.graphics_item_set(self.target_GI)

    def play(self, name):
        if self.sounds_enabled[name]:
            self.sounds[name].play()

    def set_sounds(self, action, name):
        if action.isChecked():
            self.sounds_enabled[name] = True
        else:
            self.sounds_enabled[name] = False

    # TODO: dodelat
    def view_show_window(self, action, dock_widget):
        if action.isChecked():
            dock_widget.show()
        else:
            dock_widget.hide()

    # TODO: dodelat
    def actionAutoguider_triggered(self, checked):
        widget = self.toolBar.widgetForAction(self.actionAutoguider)

        if checked:
            self.actionAutoguider.setIconText("Autoguider ON")
            widget.setStyleSheet("background-color: #CCFFCC;")

            # DBG
            self.last_fits = ""
            self.autoguider_on = True
        else:
            self.actionAutoguider.setIconText("Autoguider OFF")
            widget.setStyleSheet("background-color: #FFCCCC;")
            self.autoguider_on = False

    # TODO: dodelat
    def actionCamerasPower_triggered(self, checked):
        widget = self.toolBar.widgetForAction(self.actionCamerasPower)

        if checked:
            self.actionCamerasPower.setIconText("Cameras power ON")
            widget.setStyleSheet("background-color: #CCFFCC;")
        else:
            answer = QMessageBox.question(self, "WARNING",
                "WARNING: Do you really want to cameras power off?", QMessageBox.Yes | QMessageBox.No)
            if answer == QMessageBox.No:
                self.actionCamerasPower.setChecked(True)
                return

            self.actionCamerasPower.setIconText("Cameras power OFF")
            widget.setStyleSheet("background-color: #FFCCCC;")

    def scale_interval_set(self, idx):
        self.scale_interval = self.scale_interval_from_text[self.scale_interval_CB.itemText(idx)]
        self.reaload_pixmap()

    def scale_stretch_set(self, idx):
        self.scale_stretch = self.scale_stretch_from_text[self.scale_stretch_CB.itemText(idx)]
        self.reaload_pixmap()

    # TODO: zkontrolovat zda-li se target a source vejde do nove nacteneho FITS
    def reaload_pixmap(self):
        height, width = self.image_orig.shape
        bytes_per_line = width

        self.scene.setSceneRect(0, 0, width - self.target_GI.width, height - self.target_GI.height)

        image_normalize = ImageNormalize(self.image_orig, interval=self.scale_interval, stretch=self.scale_stretch, clip=True)
        image = img_as_ubyte(image_normalize(self.image_orig))

        # QImage.Format_Grayscale16 - The image is stored using an 16-bit grayscale format. (added in Qt 5.13)
        qimage = QImage(image, width, height, bytes_per_line, QImage.Format_Grayscale8)
        pixmap = QPixmap(qimage)

        self.graphics_pixmap_item.setPixmap(pixmap)

    def inter_image_guiding_CHB_changed(self, idx):
        value = self.inter_image_guiding_CHB.isChecked()

        self.expose_exptime_pointing_LB.setEnabled(value)
        self.expose_exptime_pointing_DSB.setEnabled(value)

    def expose_enable_preflash_CHB_changed(self, idx):
        value = self.expose_enable_preflash_CHB.isChecked()

        if value:
            self.expose_preflash_DSB.setValue(1)
        else:
            self.expose_preflash_DSB.setValue(0)

        self.expose_preflash_LB.setEnabled(value)
        self.expose_preflash_DSB.setEnabled(value)
        self.expose_preflash_num_clear_LB.setEnabled(value)
        self.expose_preflash_num_clear_SB.setEnabled(value)

    def expose_camera_changed(self, idx):

        if idx == 0:
            # G1
            self.camera_name = "pointing"
            self.set_expose_camera_pointing()
        else:
            # G2
            self.camera_name = "photometric"
            self.set_expose_camera_photometry()

        self.target_x_SB.setValue(self.target_x)
        self.target_y_SB.setValue(self.target_y)
        self.source_x_SB.setValue(0)
        self.source_y_SB.setValue(0)

    def expose_ccd_temp_changed(self):
        result = self.proxy["%s_camera" % self.camera_name].set_temperature(self.expose_ccd_temp_SB.value())

        self.log("temp = %i => %s" % (self.expose_ccd_temp_SB.value(), result), "highlight")

        self.play("set")

    def expose_binning_changed(self, idx):
        binning = [1, 2, 4]
        value = binning[idx]

        result = self.proxy["%s_camera" % self.camera_name].set_binning(value, value)

        self.log("binning = %i => %s" % (value, result))

    # TODO: osetrit caste volani za sebou
    def expose_preflash_changed(self):
        preflash_time = self.expose_preflash_DSB.value()
        clear_num = self.expose_preflash_num_clear_SB.value()

        result = self.proxy["%s_camera" % self.camera_name].set_preflash(preflash_time, clear_num)

        self.log("set_preflash(%f, %i) => %s" % (preflash_time, clear_num, result))

    def set_hide_photometry_widgets(self, hide):
        widgets = [
            self.expose_exptime_scientific_LB,
            self.expose_exptime_scientific_DSB,
            self.expose_ccd_temp_LB,
            self.expose_ccd_temp_SB,
            self.expose_filter_GB,
            self.inter_image_guiding_CHB,
            self.expose_binning_CB,
            self.expose_binning_LB,
            self.expose_preflash_num_clear_LB,
            self.expose_preflash_num_clear_SB,
            self.expose_preflash_LB,
            self.expose_preflash_DSB,
            self.expose_enable_preflash_CHB,
        ]

        for key in self.expose_filter_checkboxes:
            widgets.append(self.expose_filter_checkboxes[key])

        fce = methodcaller("show")
        if hide:
            fce = methodcaller("hide")

        for widget in widgets:
            fce(widget)

    def set_expose_camera_pointing(self):
        # G1 - jine zrcatko
        # Field size: 7.09528 x 5.33268 arcminutes
        # Field rotation angle: up is -135.643 degrees E of N
        # Field parity: pos
        # pixel scale 0.649074 arcsec/pix

        # 2019-12-18
        # Field: 2019-12-18_19-00-57_001.fit
        # Field size: 6.98142 x 5.27808 arcminutes
        # Field rotation angle: up is -135.758 degrees E of N
        # Field parity: pos
        # pixel scale 0.639643 arcsec/pix.

        #self.field_rotation_angle = 45.903
        #self.field_parity = 1
        #self.pix2arcsec = 0.605898

        # field_rotation_angle = 180 + frd
        self.field_rotation_angle = self.cfg["pointing_camera"]["field_rotation_angle"]
        self.field_parity = self.cfg["pointing_camera"]["field_parity"]
        self.pix2arcsec = self.cfg["pointing_camera"]["pix2arcsec"]
        self.target_x = self.cfg["pointing_camera"]["target_x"]
        self.target_y = self.cfg["pointing_camera"]["target_y"]

        self.field_rotation_angle_DSB.setValue(self.field_rotation_angle)
        self.field_parity_CB.setCurrentIndex(0)
        self.pixels2arcsec_DSB.setValue(self.pix2arcsec)

        self.set_hide_photometry_widgets(True)

        self.read_mode_CB.clear()
        for item in self.cfg["pointing_camera"]["read_modes"]:
            self.read_mode_CB.addItem(item)

    def set_expose_camera_photometry(self):
        # G2 - neni pro binning 1x1
        # Field size: 7.34287 x 4.93668 arcminutes
        # Field rotation angle: up is -45.903 degrees E of N
        # Field parity: neg
        # pixel scale 0.605898 arcsec/pix

        # 2019-09-21
        # Field: 2019-09-21_18-38-00_001.fit
        # Field size: 7.33896 x 4.94626 arcminutes
        # Field rotation angle: up is -45.7991 degrees E of N
        # Field parity: neg
        # pixel scale 0.201665 arcsec/pix.

        # 2020-01-06
        # Field: f202001060162.fit
        # Field size: 7.34397 x 4.96184 arcminutes
        # Field rotation angle: up is -46.0547 degrees E of N
        # Field parity: neg
        # pixel scale 0.202102 arcsec/pix.

        #self.field_rotation_angle = -135.643
        #self.field_parity = -1
        #self.pix2arcsec = 0.18

        # field_rotation_angle = -90 + frd
        self.field_rotation_angle = self.cfg["photometric_camera"]["field_rotation_angle"]
        self.field_parity = self.cfg["photometric_camera"]["field_parity"]
        self.pix2arcsec = self.cfg["photometric_camera"]["pix2arcsec"]
        self.target_x = self.cfg["photometric_camera"]["target_x"]
        self.target_y = self.cfg["photometric_camera"]["target_y"]

        self.field_rotation_angle_DSB.setValue(self.field_rotation_angle)
        self.field_parity_CB.setCurrentIndex(1)
        self.pixels2arcsec_DSB.setValue(self.pix2arcsec)

        self.set_hide_photometry_widgets(False)

        self.read_mode_CB.clear()
        for item in self.cfg["photometric_camera"]["read_modes"]:
            self.read_mode_CB.addItem(item)

    def source_autodetect_toggled(self):

        if self.source_autodetect_CB.isChecked():
            self.source_x_SB.setEnabled(False)
            self.source_y_SB.setEnabled(False)
        else:
            self.source_x_SB.setEnabled(True)
            self.source_y_SB.setEnabled(True)

    def expose_start_clicked(self):
        print("expose_start_clicked")

        # G1 - exptime_min = 0.000125

        # G2 - exptime_min = 0.1

        count_repeat = self.expose_count_repeat_SB.value()

        if self.camera_name == "photometric":
            expose_time = self.expose_exptime_scientific_DSB.value()
            expose_time_pointing = -1
            read_mode = self.read_mode_CB.currentIndex()
            filters = []

            for key in self.expose_filter_checkboxes:
                if self.expose_filter_checkboxes[key].isChecked():
                    filters.append(int(key.split(" ")[0]))

            if self.inter_image_guiding_CHB.isChecked():
                expose_time_pointing = self.expose_exptime_pointing_DSB.value()
        else:
            expose_time = -1
            expose_time_pointing = self.expose_exptime_pointing_DSB.value()
            read_mode = 0
            filters = []

        self.proxy["%s_camera" % self.camera_name].start_exposure(expose_time, expose_time_pointing,
            count_repeat, self.expose_delay_after_exposure_SB.value(),
            self.expose_object_CB.currentText(), self.expose_target_LE.text(), self.expose_observers_LE.text(),
            read_mode, filters)

    def expose_stop_clicked(self):
        print("expose_stop_clicked")

        self.proxy["%s_camera" % self.camera_name].stop_exposure()

    def move_source_to_target_clicked(self):
        print("GO to target")

        source_pos = self.source_GI.scenePos()
        target_pos = self.target_GI.scenePos()

        x = target_pos.x() - source_pos.x()
        y = target_pos.y() - source_pos.y()

        self.telescope_offset(x, y)

    def flats_start(self):
        self.log("Flats start")

        try:
            self.flats_thread = FlatsThread(self)
        except:
            exctype, value = sys.exc_info()[:2]
            self.log("%s: %s" % (exctype, value), "error")
            return

        self.flats_stop_event.clear()

        flats_worker = Worker(self.flats_thread.loop) # Any other args, kwargs are passed to the run function
        flats_worker.signals.result.connect(self.flats_thread.print_output)
        flats_worker.signals.finished.connect(self.flats_thread.thread_complete)
        flats_worker.signals.progress.connect(self.flats_thread.progress_fn)
        flats_worker.signals.error.connect(self.flats_thread.error_fn)

        self.threadpool.start(flats_worker)

    def flats_stop(self):
        self.log("Flats stop")
        self.flats_stop_event.set()

    def telescope_move(self, action):
        step = self.telescope_move_DSB.value()
        x = 0.0
        y = 0.0

        if action == "up":
            y = step
        elif action == "down":
            y = -step
        elif action == "left":
            x = -step
        elif action == "right":
            x = step

        xt, yt = self.field_rotation(x, y)

        self.log("telescope %s %.1f %.1f" % (action, xt, yt), "highlight")

        self.run_tsgc(xt, yt)

    def field_rotation(self, x, y):
        self.field_rotation_angle = self.field_rotation_angle_DSB.value()

        if self.field_parity_CB.currentIndex() == 0:
            self.field_parity = 1
        else:
            self.field_parity = -1

        angle_rad = math.radians(self.field_rotation_angle)

        xt = x * math.cos(angle_rad) - y * math.sin(angle_rad)
        yt = x * math.sin(angle_rad) + y * math.cos(angle_rad)

        xt *= self.field_parity

        return [xt, yt]

    def run_tsgc(self, x, y):
        cmd = ["/opt/opso/current/bin/ps_tsgc", "%.1f" % x, "%.1f" % y ]
        print(cmd)

        try:
            output = subprocess.check_output(cmd)
            print(output)
        except:
            traceback.print_exc()

    def telescope_offset(self, x, y, autoguider=False, fwhm=None):
        print("telescope_offset", x, y)

        self.pix2arcsec = self.pixels2arcsec_DSB.value()

        xt, yt = self.field_rotation(x, y)

        print("offset %i %i px" % (x, y))
        print("offset rotated %.1f %.1f px" % (xt, yt))

        xt *= self.pix2arcsec
        yt *= self.pix2arcsec

        print('offset rotated %.1f %.1f "' % (xt, yt))

        if not autoguider or self.is_telescope_move_acceptable(xt, yt, fwhm):
            self.run_tsgc(xt, yt)

    def is_telescope_move_acceptable(self, x_offset, y_offset, fwhm):
        dt_str = datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")

        message = '%s - telescope move %.1f" %.1f"' % (dt_str, x_offset, y_offset)
        result = True
        message_color = "CCFFCC"

        max_star_movement = self.autoguider_max_star_movement_SB.value()
        min_star_fwhm = self.autoguider_min_star_fwhm_SB.value()
        max_star_fwhm = self.autoguider_max_star_fwhm_SB.value()

        if abs(x_offset) > max_star_movement:
            message = '%s is not acceptable because x_offset abs(%.1f") > %i"' % (message, x_offset, max_star_movement)
            result = False
        elif abs(y_offset) > max_star_movement:
            message = '%s is not acceptable because y_offset abs(%.1f") > %i"' % (message, y_offset, max_star_movement)
            result= False
        else:
            for value in fwhm:
                if abs(value) > max_star_fwhm:
                    message = '%s is not acceptable because FWHM abs(%.1f pix) > %i pix' % (message, value, max_star_fwhm)
                    result = False
                    break
                elif abs(value) < min_star_fwhm:
                    message = '%s is not acceptable because FWHM abs(%.1f pix) < %i pix' % (message, value, min_star_fwhm)
                    result = False
                    break

        self.statusbar.showMessage(message)

        if not result:
            message_color = "FFCCCC"

        self.statusbar.setStyleSheet("background-color: #%s;" % message_color)

        return result

    def scale_change(self):
        scale = self.scale_DSB.value() * (1.0 / self.scale_previous)

        print("scale_previous = %.2f, scale = %.2f" % (self.scale_previous, scale))

        self.scale_previous = self.scale_DSB.value()

        #self.graphicsView.fitInView()

        self.graphicsView.scale(scale, scale)

    def target_size_SB_valueChanged(self):
        size = self.target_size_SB.value()

        self.target_GI.set_width_height(size, size)
        #self.source_GI.set_width_height(size, size)
        self.autodetect_GI.set_width_height(size, size)

    def graphics_item_set(self, graphics_item, fce_value=None, fce_set=None):

        is_star_detected = False
        position = graphics_item.scenePos()
        height, width = self.image_orig.shape

        # TODO: je treba predat spravne souradnice
        size = int(graphics_item.width / 2)
        x = int(position.x()) + size
        y = int(position.y()) + size

        print(x, y, size)

        try:
            subimage = self.image_orig[y-size:y+size+1, x-size:x+size+1]
            print(subimage)
            x2dg, y2dg = centroid_2dg(subimage)
            print("centroid_2dg = [%.1f; %.1f]" % (x2dg, y2dg))

            gfit = fit_2dgaussian(subimage)
            print(gfit)
            x_fwhm = gfit.x_stddev * gaussian_sigma_to_fwhm
            y_fwhm = gfit.y_stddev * gaussian_sigma_to_fwhm
            print("FWHM", x_fwhm, y_fwhm)

            x_fwhm_arcsec = x_fwhm * self.pixels2arcsec_DSB.value()
            y_fwhm_arcsec = y_fwhm * self.pixels2arcsec_DSB.value()

            self.fwhm_x_LB.setText('%.1f pix, %.1f"' % (x_fwhm, x_fwhm_arcsec))
            self.fwhm_y_LB.setText('%.1f pix, %.1f"' % (y_fwhm, y_fwhm_arcsec))
            self.gaussian_amplitude_LB.setText("%.1f" % gfit.amplitude.value)

            if not np.isnan(x2dg) and not np.isnan(y2dg):
                is_star_detected = True
                self.star_matplotlib_canvas.plot_star(subimage, x2dg, y2dg)

                star_x = position.x() + (int(x2dg) - size)
                star_y = position.y() + (int(y2dg) - size)

                if graphics_item is self.target_GI:
                    self.autodetect_GI.setX(star_x)
                    self.autodetect_GI.setY(star_y)

                    print(self.last_fits_imagetype)
                    if (self.autoguider_on and \
                        ((self.camera_name == "pointing") or (self.last_fits_imagetype == "autoguider"))):
                        target_pos = self.target_GI.scenePos()

                        x = target_pos.x() - star_x
                        y = target_pos.y() - star_y

                        fwhm = [x_fwhm, y_fwhm]
                        self.telescope_offset(x, y, autoguider=True, fwhm=fwhm)

                elif graphics_item is self.source_GI and self.source_autodetect_CB.isChecked():
                    graphics_item.setX(star_x)
                    graphics_item.setY(star_y)
        except:
            traceback.print_exc()
            QMessageBox.warning(self, "ERROR", traceback.format_exc())

        if (graphics_item.isUnderMouse()):
            print("isUnderMouse")
            return

        if (fce_value is None) or (fce_set is None):
            print("fce_value or fce_set is None")
            return

        value = fce_value()
        print("graphics_item_set", value, position.x(), position.y())
        fce_set(value)

    def load_fits(self, filename):
        hdulist = fits.open(filename)

        image = hdulist[0].data
        image = img_as_ubyte(image)

        #image = rotate(image, 180)

        #self.height, self.width = self.image.shape

        return image

    def log(self, message, level="info"):
        dt_str = datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")

        brush = QBrush(QColor.fromRgb(self.COLORS[level]))

        char_format = self.log_PTE.currentCharFormat()
        char_format.setForeground(brush)

        self.log_PTE.setCurrentCharFormat(char_format)

        self.log_PTE.appendPlainText("%s - %s" % (dt_str, message))

        if level == "error":
            self.logger.error(message)
        elif level == "warning":
            self.logger.warning(message)
        else:
            self.logger.info(message)

    def progress_telescope_command_fn(self, data):
        self.log("telescope_command(%s)" % data["command"])

    # https://davidamos.dev/the-right-way-to-compare-floats-in-python/
    def progress_guider_read_fn(self, data):
        status = "expose"

        if math.isclose(data["ccd_exposure"], 0.0):
            status = "ready"

        self.set_label_text(self.g1_state_LB, status)

        remaining_time = data["ccd_exposure"]
        exposure_time = self.expose_exptime_pointing_DSB.value()
        elapsed_time = exposure_time - remaining_time
        percent = elapsed_time / (exposure_time / 100.0)

        # TODO
        #self.expose_PB.setFormat("%s (%i/%i)" % (status, cameras_status[self.camera_name]["exposure_number"], self.expose_count_repeat_SB.value()))
        self.expose_PB.setFormat(status)

        if (status == "ready"):
            self.expose_remaining_LB.setText("0:00:00")
            self.expose_elapsed_LB.setText("0:00:00")
            self.expose_PB.setValue(0)
        else:
            self.expose_remaining_LB.setText(str(timedelta(seconds=round(remaining_time))))
            self.expose_elapsed_LB.setText(str(timedelta(seconds=round(elapsed_time))))
            self.expose_PB.setValue(percent)

        storage_cfg = self.cfg["%s_storage" % self.camera_name]

        # DBG begin
        if not self.last_fits:
            pass
            #self.last_fits = "/home/fuky/_tmp/e152/fitswkUgDe.fits"
            #self.last_fits = "/home/fuky/_tmp/e152/fitsxOUbXm.fits"
            #self.last_fits = "/home/fuky/_tmp/e152/fitsEsPvab.fits"
            #self.last_fits = "/home/fuky/_tmp/e152/fitsSuShql.fits"
            #self.last_fits = "/home/fuky/_tmp/e152/"
            #self.image_orig = self.load_fits(self.last_fits)
            #self.reaload_pixmap()
            #self.graphics_item_set(self.target_GI)

        if data["fits"] is None:
            print("no data")
            return

        image_format = self.image_format_CB.currentText()

        if image_format.startswith("JPG"):
            pass
            #quality = 95 # high
            #if image_format.find("medium") != -1:
            #    quality = 70
            #elif image_format.find("low") != -1:
            #    quality = 50

            #jpg_binary = self.proxy["%s_storage" % self.camera_name].fiber_pointing_get_jpg(quality, "")

            #if jpg_binary:
            #    self.last_fits_filename = last_fits_filename
            #    jpg_array = np.frombuffer(jpg_binary.data, np.uint8)
            #    self.image_orig = cv2.imdecode(jpg_array, cv2.IMREAD_GRAYSCALE)
            #    self.reaload_pixmap()
            #    self.graphics_item_set(self.target_GI)
            #    self.play("new_image")

            #    if storage_cfg["save"]:
            #        filename = os.path.join(storage_cfg["data_dir"], last_fits_filename.replace(".fit", ".jpg"))
            #        print(filename)
            #        fo = open(filename, "wb")
            #        fo.write(jpg_binary.data)
            #        fo.close()

            #    self.image_size_LB.setText(humanize.naturalsize(jpg_array.nbytes))
        else:
            fits_fo = io.BytesIO(data["fits"])
            hdulist = fits.open(fits_fo)
            prihdr = hdulist[0].header
            # 2022-10-16T05:33:02.484
            date_obs = prihdr["DATE-OBS"].replace(":", "_").replace(".", "_")
            print(date_obs)
            self.last_fits_filename = date_obs
            filename = os.path.join(storage_cfg["data_dir"], "%s.fit" % date_obs)

            self.image_orig = img_as_ubyte(hdulist[0].data)
            self.reaload_pixmap()
            self.graphics_item_set(self.target_GI)
            self.play("new_image")

            if self.scan_dialog is not None:
                self.scan_dialog.load_image(self.image_orig)

            if storage_cfg["save"]:
                print(filename)
                fo = open(filename, "wb")
                fo.write(data["fits"])
                fo.close()

            if storage_cfg["ds9_bin"]:
                self.ds9_xpaset(filename, storage_cfg)

            self.image_size_LB.setText("")

    def progress_guider_command_fn(self, data):
        if data["command"] == "gain":
            msg = "Set gain from %(old_gain)i to %(new_gain)i" % data
            self.log(msg)

    def thread_progress(self, category, data):
        if self.exit.is_set():
            return

        if category == "msg":
            self.refresh_statusbar(ClientStatus.RUNNING, data["msg"])
        elif category in self.progress_callback_dict:
            self.logger.debug("thread_progress(category = %s, data = %s)" % (category, data))
            self.progress_callback_dict[category](data)
            #self.refresh_statusbar(ClientStatus.RUNNING, category)
            self.refresh_statusbar(ClientStatus.RUNNING)

    def thread_result(self, name, s):
        if self.exit.is_set():
            return

        self.logger.info("%s = %s" % (name, s))

    def thread_error(self, name, error):
        if self.exit.is_set():
            return

        dt_str = datetime.now().strftime("%H:%M:%S")
        exctype, value, format_exc = error

        self.refresh_statusbar(ClientStatus.FAILED, "%s %s" % (exctype, value))

        self.logger.error("exctype = %s" % exctype)
        self.logger.error("value = %s" % value)
        self.logger.error("format_exc = %s" % format_exc)

        self.log_PTE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_PTE.textCursor().insertHtml('<font color="#FF0000">%s</font><br>' % value)

    def thread_finished(self, name):
        if self.exit.is_set():
            self.logger.info("%s COMPLETE! Exiting..." % name)
            return

        if self.status != ClientStatus.FAILED:
            self.refresh_statusbar(ClientStatus.SUCCESS)

        self.logger.info("%s COMPLETE!" % name)

    def is_stop(self):

        if self.exit.is_set() or self.stop_event.is_set():
            return True

        return False

def main():
    app = QApplication([])
    fiber_pointing_ui = FiberPointingUI()

    return app.exec()

if __name__ == '__main__':
    main()
