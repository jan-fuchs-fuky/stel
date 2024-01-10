#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2022-2023 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# $ ssh -L 9999:alhena:9999 -L 8888:alhena:8888 -L 5001:almisan:5001 -L 5002:alhena:5002 primula
#

# TODO:
#
#     - aktualizace progress_bar expozimetru: lepsi reseni je stejne jako v
#     pripade exposure_time, tj. nacitani primo z damonu, ktery ridi CCD,
#     protoze pak se zobrazi vse korektne i v klientu zapnutem az v prubehu
#     expozice
#

import os
import sys
import re
import time
import json
import requests
import configparser
import traceback
import xmlrpc.client
import threading
import logging
import numpy as np

from operator import methodcaller
from enum import Enum
from datetime import datetime, timezone, timedelta
from logging.handlers import RotatingFileHandler

from astroplan.plots import plot_airmass, plot_sky
from astroplan import FixedTarget, Observer

from matplotlib.figure import Figure
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg

from astropy.coordinates import EarthLocation, SkyCoord, Angle
from astropy.time import Time
from astropy import units as u
from astropy.coordinates import AltAz

from astroquery.simbad import Simbad

from PyQt5 import (
    uic,
    QtCore,
)

from PyQt5.QtWidgets import (
    QMessageBox,
    QMainWindow,
    QPushButton,
    QApplication,
    QComboBox,
    QSpinBox,
    QDoubleSpinBox,
    QTableWidget,
    QTableWidgetItem,
    QLineEdit,
    QDialog,
    QDialogButtonBox,
    QVBoxLayout,
    QLabel,
    QAbstractItemView,
    QCheckBox,
    QFileDialog,
    QSizePolicy,
)

from PyQt5.QtCore import (
    QRunnable,
    QThreadPool,
    QObject,
    pyqtSlot,
    pyqtSignal,
    Qt,
    QDir,
)

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

OBSERVE_CLIENT_CFG = "/opt/observe_batch/etc/observe_client.cfg"

CCD_CONTROL_UI = "%s/../share/ccd_control.ui" % SCRIPT_PATH

CCD_CONTROL_CFG = "%s/../etc/ccd_control.cfg" % SCRIPT_PATH

CCD_CONTROL_LOG = "%s/../log/ccd_control_%%s.log" % SCRIPT_PATH

def init_logger(logger, filename):
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d][%(thread)d] - %(levelname)s - %(message)s")

    # DBG
    #formatter = logging.Formatter(
    #    ("%(asctime)s - %(name)s[%(process)d][%(thread)d] - %(levelname)s - "
    #     "%(filename)s:%(lineno)s - %(funcName)s() - %(message)s - "))

    fh = RotatingFileHandler(filename, maxBytes=10485760, backupCount=10)
    #fh.setLevel(logging.INFO)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)

    #logger.setLevel(logging.INFO)
    logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

class ClientStatus(Enum):

    IDLE = 0
    RUNNING = 1
    SUCCESS = 2
    FAILED = 3

# TODO: rozpracovano
class CoordinatesStatus(Enum):

    UNKNOWN = 0
    PREPARED = 1
    MOVING = 2
    SUCCESS = 3

class WorkerSignals(QObject):
    '''
    Defines the signals available from a running worker thread.

    Supported signals are:

    finished
        No data

    error
        `tuple` (exctype, value, traceback.format_exc())

    result
        `object` data returned from processing, anything

    progress
        `int` indicating % progress

    '''
    finished = pyqtSignal(str)
    error = pyqtSignal(str, tuple)
    result = pyqtSignal(str, object)
    progress = pyqtSignal(str, object)

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
        self.thread_exit = kwargs.pop("thread_exit")

        # Add the callback to our kwargs
        self.kwargs['progress_callback'] = self.signals.progress

        gui = kwargs.pop("gui")
        self.signals.result.connect(gui.thread_result)
        self.signals.finished.connect(gui.thread_finished)
        self.signals.progress.connect(gui.thread_progress)
        self.signals.error.connect(gui.thread_error)

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
            self.signals.error.emit(self.kwargs["name"], (exctype, value, traceback.format_exc()))
        else:
            self.signals.result.emit(self.kwargs["name"], result) # Return the result of the processing
        finally:
            self.thread_exit.set()
            self.signals.finished.emit(self.kwargs["name"]) # Done

class TelescopeThread:
    GLST = {
        "state_of_telescope": {
            "0": "OFF",
            "1": "STOP",
            "2": "TRACK",
            "3": "SLEW",
            "4": "SLEWHADA",
            "5": "SYNC",
            "6": "PARK",
        },

        "dome": {
            "0": "STOP",
            "1": "PLUS",
            "2": "MINUS",
            "3": "AUTO_STOP",
            "4": "AUTO_PLUS",
            "5": "AUTO_MINUS",
            "6": "SYNC",
            "7": "SLEW_MINUS",
            "8": "SLEW_PLUS",
            "9": "SLIT",
        },

        "slit": {
            "0": "unknown",
            "1": "opening",
            "2": "closing",
            "3": "opened",
            "4": "closed",
        },

        "mirror_cover": {
            "0": "unknown",
            "1": "opening",
            "2": "closing",
            "3": "opened",
            "4": "closed",
        },

        "focus": {
            "0": "stopped",
            "1": "manual minus",
            "2": "manual plus",
            "3": "positioning",
        },

        "status_bits1": {},
        "status_bits2": {},
    }

    def __init__(self, gui):
        self.gui = gui
        self.cfg = gui.cfg
        self.is_stop = gui.is_stop

        self.telescope_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["telescope"])

    def ascol_send(self, cmd, frequent=False):
        if frequent:
            logger_fce = self.logger.debug
        else:
            logger_fce = self.logger.info

        answer = self.telescope_proxy.telescope_execute(cmd)

        if answer != "ERR":
            logger_fce("ASCOL '%s' => '%s'" % (cmd, answer))
        else:
            self.logger.error("ASCOL '%s' => '%s'" % (cmd, answer))

        return answer

    def run_read(self):

        while not self.is_stop():
            # TODO: osetrit pripadnou vyjimku
            status = self.telescope_proxy.telescope_info()

            self.progress_callback.emit(self.name, status)
            time.sleep(1)

    def run_command(self, command):
        for item in command:
            result = self.ascol_send(item)

            data = {
                "command": item,
                "result": result,
            }

            self.progress_callback.emit(self.name, data)
            time.sleep(0.1)

    def run(self, progress_callback, name, command):
        self.progress_callback = progress_callback
        self.name = name

        self.logger = logging.getLogger("telescope_%s" % name)
        init_logger(self.logger, CCD_CONTROL_LOG % name)
        self.logger.info("Starting process '%s'" % name)

        if name == "telescope_read":
            self.run_read()
        else:
            self.run_command(command)

        return "Done."

class SpectrographThread:

    def __init__(self, gui):
        self.gui = gui
        self.cfg = gui.cfg
        self.is_stop = gui.is_stop

        self.spectrograph_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["spectrograph"])

    def ascol_send(self, cmd, frequent=False):
        if frequent:
            logger_fce = self.logger.debug
        else:
            logger_fce = self.logger.info

        answer = self.spectrograph_proxy.spectrograph_execute(cmd)

        if answer != "ERR":
            logger_fce("ASCOL '%s' => '%s'" % (cmd, answer))
        else:
            self.logger.error("ASCOL '%s' => '%s'" % (cmd, answer))

        return answer

    def run_read(self):

        while not self.is_stop():
            # TODO: osetrit pripadnou vyjimku
            status = self.spectrograph_proxy.spectrograph_info()

            self.progress_callback.emit(self.name, status)
            time.sleep(1)

    def run_command(self, command):
        for item in command:
            result = self.ascol_send(item)

            data = {
                "command": item,
                "result": result,
            }

            self.progress_callback.emit(self.name, data)
            time.sleep(0.1)

    def run(self, progress_callback, name, command):
        self.progress_callback = progress_callback
        self.name = name

        self.logger = logging.getLogger("spectrograph_%s" % name)
        init_logger(self.logger, CCD_CONTROL_LOG % name)
        self.logger.info("Starting process '%s'" % name)

        if name == "spectrograph_read":
            self.run_read()
        else:
            self.run_command(command)

        return "Done."

class CCDThread:

    def __init__(self, gui, ccd):
        self.gui = gui
        self.ccd = ccd
        self.cfg = gui.cfg
        self.is_stop = gui.is_stop

        self.proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg[self.ccd])

        self.proxy_callbacks = {
            "set": self.proxy.expose_set,
            "set_key": self.proxy.expose_set_key,
            "start": self.proxy.expose_start,
            "readout": self.proxy.expose_readout,
            "time_update": self.proxy.expose_time_update,
            "meter_update": self.proxy.expose_meter_update,
        }

    def run_read(self):

        while not self.is_stop():
            # TODO: osetrit pripadnou vyjimku
            status = self.proxy.expose_info()

            self.progress_callback.emit(self.name, status)
            time.sleep(1)

    def run_command(self, command):

        for item in command:
            result = self.proxy_callbacks[item[0]](*item[1:])

            data = {
                "command": item,
                "result": result,
            }

            self.progress_callback.emit(self.name, data)

    def run(self, progress_callback, name, command):
        self.progress_callback = progress_callback
        self.name = name

        self.logger = logging.getLogger(name)
        init_logger(self.logger, CCD_CONTROL_LOG % name)
        self.logger.info("Starting process '%s'" % name)

        if name.endswith("_read"):
            self.run_read()
        else:
            self.run_command(command)

        return "Done."

class MatplotlibCanvas(FigureCanvasQTAgg):
    """Ultimately, this is a QWidget (as well as a FigureCanvasAgg, etc.)."""

    def __init__(self, parent=None, width=4, height=4, dpi=100, polar=False):
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111, polar=polar)

        FigureCanvasQTAgg.__init__(self, fig)
        self.setParent(parent)

        FigureCanvasQTAgg.setSizePolicy(self,
                                   QSizePolicy.Expanding,
                                   QSizePolicy.Expanding)
        FigureCanvasQTAgg.updateGeometry(self)

class StarMatplotlibCanvas(MatplotlibCanvas):

    def __init__(self, parent=None, width=4, height=4, dpi=100):
        super(StarMatplotlibCanvas, self).__init__(parent, width, height, dpi, polar=False)

    def plot_star(self, target, time, observer):
        self.axes.cla()

        plot_airmass(target, observer, time, self.axes, brightness_shading=True, altitude_yaxis=True)
        self.draw()

class LimitsMatplotlibCanvas(MatplotlibCanvas):

    def __init__(self, parent=None, width=4, height=4, dpi=100):
        super(LimitsMatplotlibCanvas, self).__init__(parent, width, height, dpi, polar=True)

    def plot_star(self, coord, time, observer):
        self.axes.cla()

        target = FixedTarget(coord=coord, name="Sirius")
        style = {"color": "r"}

        # Every call to the function astroplan.plots.plot_sky raises an exception.
        # https://github.com/astropy/astroplan/issues/468
        # ValueError: The number of FixedLocator locations (8), usually from a call to set_ticks, does not match the number of ticklabels (7).
        # SOLUTION: https://github.com/astropy/astroplan/pull/494/commits/161be86047dbf3abdf4ade926a179f4aad05ac2b
        # # dpkg-divert --divert /usr/lib/python3/dist-packages/astroplan/plots/sky.py.orig --rename /usr/lib/python3/dist-packages/astroplan/plots/sky.py
        # $ vim /usr/lib/python3/dist-packages/astroplan/plots/sky.py
        #     216c216
        #     <     ax.set_thetagrids(range(0, 360, 45), theta_labels)
        #     ---
        #     >     ax.set_thetagrids(range(0, 315, 45), theta_labels)
        plot_sky(target, observer, time, self.axes, style_kwargs=style)
        self.draw()

# TODO: sdilet kod s observe_client.py
class SelectTargetDialog(QDialog):

    def __init__(self, targets, parent=None):
        super().__init__(parent=parent)

        self.setWindowTitle("CCD_control: Select Target")

        buttons = QDialogButtonBox.Ok | QDialogButtonBox.Cancel

        self.button_box = QDialogButtonBox(buttons)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self.columns = {
            'Object': 0,
            'Names': 1,
            'V mag': 2,
            'RA': 3,
            'DEC': 4,
            'Alt': 5,
            'HA': 6,
            'Supervisors': 7,
            'Spectrum': 8,
        }

        conv = {
            'object': 'Object',
            'names': 'Names',
            'v_mag': 'V mag',
            'ra': 'RA',
            'de': 'DEC',
            'alt': 'Alt',
            'ha': 'HA',
            'svi': 'Supervisors',
            'spectrum': 'Spectrum',
        }

        targets_len = len(targets)

        self.targets_TW = QTableWidget()
        self.targets_TW.setSelectionMode(QAbstractItemView.SingleSelection)
        self.targets_TW.setColumnCount(len(self.columns))
        self.targets_TW.setRowCount(targets_len)
        self.targets_TW.setHorizontalHeaderLabels(self.columns.keys())
        self.targets_TW.setSelectionBehavior(QTableWidget.SelectRows)
        self.targets_TW.setEditTriggers(QTableWidget.NoEditTriggers)

        for row, target in enumerate(targets):
            for key in target:
                column_name = conv[key]
                column = self.columns[column_name]
                value = target[key]

                if isinstance(value, float):
                    value = "%.1f" % value

                self.targets_TW.setItem(row, column, QTableWidgetItem(value))

        if targets_len > 0:
            self.targets_TW.setCurrentCell(0, 0)

        self.layout = QVBoxLayout()
        self.layout.addWidget(self.targets_TW)
        self.layout.addWidget(self.button_box)
        self.setLayout(self.layout)

        # TODO: vyresit lepe
        self.resize(2000, 1080)

    def get_selected_target_idx(self):
        current_row = self.targets_TW.currentRow()

        return current_row

# TODO: sdilet kod s observe_client.py
class SelectTargetThread:

    def __init__(self, gui):
        self.gui = gui

    def run(self, progress_callback, name, target):
        params = {
            "sname": target,
            "out": "json",
        }

        auth = ("public", "db2m")

        response = requests.get("https://stelweb.asu.cas.cz/stars/startab.html", params=params, auth=auth)

        if response.status_code != 200:
            raise Exception("ERROR: response.status_code = %i" % response.status_code)

        stars = response.json()

        progress_callback.emit(name, stars)

# TODO: sdilet kod s observe_client.py
class SelectGratingAngleDialog(QDialog):

    def __init__(self, grating_angles, parent=None):
        super().__init__(parent=parent)

        self.setWindowTitle("ObserveClient: Select grating angle")

        buttons = QDialogButtonBox.Ok | QDialogButtonBox.Cancel

        self.button_box = QDialogButtonBox(buttons)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self.columns = {
            'name': 0,
            'grating_angle': 1,
            'spectral_filter': 2,
            'range': 3,
            'flat': 4,
            'comp': 5,
        }

        grating_angles_len = len(grating_angles)

        self.grating_angles_TW = QTableWidget()
        self.grating_angles_TW.setSelectionMode(QAbstractItemView.SingleSelection)
        self.grating_angles_TW.setColumnCount(len(self.columns))
        self.grating_angles_TW.setRowCount(grating_angles_len)
        self.grating_angles_TW.setHorizontalHeaderLabels(self.columns.keys())
        self.grating_angles_TW.setSelectionBehavior(QTableWidget.SelectRows)
        self.grating_angles_TW.setEditTriggers(QTableWidget.NoEditTriggers)

        row = -1
        for name in grating_angles:
            row += 1
            self.grating_angles_TW.setItem(row, self.columns["name"], QTableWidgetItem(name))

            for key in grating_angles[name]:
                column = self.columns[key]
                value = grating_angles[name][key]

                if isinstance(value, int):
                    value = "%i s" % value

                self.grating_angles_TW.setItem(row, column, QTableWidgetItem(value))

        if grating_angles_len > 0:
            self.grating_angles_TW.setCurrentCell(0, 0)

        self.layout = QVBoxLayout()
        self.layout.addWidget(self.grating_angles_TW)
        self.layout.addWidget(self.button_box)
        self.setLayout(self.layout)

        self.resize(1280, 1024)

    def get_selected_grating_angle(self):
        row = self.grating_angles_TW.currentRow()
        item = {}

        for key in self.columns:
            table_widget = self.grating_angles_TW.item(row, self.columns[key])
            item[key] = table_widget.text()

        return item

class CCDClientUI(QMainWindow):

    SPECTROGRAPH_COLLIMATOR2STR = {
        "0": "STOP",
        "1": "OPEN",
        "2": "CLOSE",
        "3": "OPEN LEFT",
        "4": "OPEN RIGHT",
        "5": "MOVING",
        "6": "TIMEOUT",
    }

    CCD700_SPECTRAL_FILTER2STR = {
        "0": "STOP",
        "1": "1",
        "2": "2",
        "3": "3",
        "4": "4",
        "5": "5",
        "6": "MOVING",
        "7": "TIMEOUT",
    }

    CCD700_GRATING_ANGLE_STATE2STR = {
        "0": "STOP",
        "1": "MOVING",
        "2": "TIMEOUT",
    }

    def __init__(self):
        super(CCDClientUI, self).__init__()

        process_name = "gui"
        self.logger = logging.getLogger("ccd_control_%s" % process_name)
        init_logger(self.logger, CCD_CONTROL_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.exit = threading.Event()
        self.stop_event = threading.Event()
        self.star_is_ready_event = threading.Event()
        self.status = ClientStatus.IDLE
        self.matplotlib_dt = datetime.now(tz=timezone.utc) - timedelta(days=1)
        self.actual_grating_angle = None

        self.thread_exit = {}
        for key in ["read", "command", "target"]:
            self.thread_exit[key] = threading.Event()
            self.thread_exit[key].set()

        self.status_color = {
            "IDLE": "#CCCCCC",
            "RUNNING": "#CCFFCC",
            "SUCCESS": "#FFFFFF",
            "FAILED": "#FFCCCC",
        }

        self.load_cfg()

        self.history = {
            "oes": {
                "target_exptime": self.cfg["oes_setup"]["target_exptime"],
                "dark_exptime": self.cfg["oes_setup"]["dark_exptime"],
                "zero_exptime": self.cfg["oes_setup"]["zero_exptime"],
                "flat_exptime": self.cfg["oes_setup"]["flat_exptime"],
                "comp_exptime": self.cfg["oes_setup"]["comp_exptime"],
            },
            "ccd700": {
                "target_exptime": self.cfg["ccd700_setup"]["target_exptime"],
                "dark_exptime": self.cfg["ccd700_setup"]["dark_exptime"],
                "zero_exptime": self.cfg["ccd700_setup"]["zero_exptime"],
                "flat_exptime": self.cfg["ccd700_setup"]["flat_exptime"],
                "comp_exptime": self.cfg["ccd700_setup"]["comp_exptime"],
            },
        }

        self.prepared_star = {
            "object": "",
            "ra": "",
            "dec": "",
            "position": "",
        }

        self.ccd700_actual_count_of_pulses_str = "0"
        self.oes_actual_count_of_pulses_str = "0"

        self.longitude = self.cfg["observer"]["longitude"] * u.deg
        self.latitude = self.cfg["observer"]["latitude"] * u.deg
        self.elevation = self.cfg["observer"]["elevation"] * u.m

        self.observer = Observer(longitude=self.longitude, latitude=self.latitude, elevation=self.elevation)
        self.earth_location = EarthLocation(lat=self.latitude, lon=self.longitude, height=self.elevation)

        self.ra_pattern = re.compile(self.cfg["pattern"]["ra"])
        self.dec_pattern = re.compile(self.cfg["pattern"]["dec"])
        self.fits_header_pattern = re.compile(self.cfg["pattern"]["fits_header"])
        self.time_h_m_s_pattern = re.compile(self.cfg["pattern"]["time_h_m_s"])
        self.time_m_s_pattern = re.compile(self.cfg["pattern"]["time_m_s"])
        self.time_s_pattern = re.compile(self.cfg["pattern"]["time_s"])
        self.count_of_pulses_pattern = re.compile(self.cfg["pattern"]["count_of_pulses"])

        uic.loadUi(CCD_CONTROL_UI, self)

        self.ccd700_widgets = [
            self.ccd700_count_repeat_SB,
            self.ccd700_exposure_time_CHB,
            self.ccd700_count_of_pulses_CHB,
            self.ccd700_object_CB,
            self.ccd700_target_LE,
            self.ccd700_start_BT,
            self.ccd700_data_path_CB,
            self.ccd700_gain_CB,
            self.ccd700_readout_speed_CB,
            self.ccd700_collimator_CB,
            self.ccd700_spectral_filter_CB,
            self.ccd700_grating_angle_LE,
            self.ccd700_load_setup_BT,
            self.ccd700_spectrograph_set_BT,
        ]

        self.oes_widgets = [
            self.oes_count_repeat_SB,
            self.oes_exposure_time_CHB,
            self.oes_count_of_pulses_CHB,
            self.oes_object_CB,
            self.oes_target_LE,
            self.oes_start_BT,
            self.oes_data_path_CB,
            self.oes_readout_speed_CB,
            self.oes_collimator_CB,
            self.oes_spectral_filter_CB,
            self.oes_slit_height_CB,
        ]

        self.ccd700_title_LB.setStyleSheet("background-color: #76EEC6;")
        self.oes_title_LB.setStyleSheet("background-color: #CCCCCC;")

        self.star_matplotlib_canvas = StarMatplotlibCanvas(self.star_matplotlib_widget, width=5, height=4, dpi=100)
        self.star_matplotlib_layout.addWidget(self.star_matplotlib_canvas)

        self.limits_matplotlib_canvas = LimitsMatplotlibCanvas(self.limits_matplotlib_widget, width=5, height=4, dpi=100)
        self.limits_matplotlib_layout.addWidget(self.limits_matplotlib_canvas)

        self.telescope_correction_model_CB.currentIndexChanged.connect(self.telescope_correction_model_changed)
        self.ccd700_spectral_filter_CB.currentIndexChanged.connect(self.ccd700_spectral_filter_changed)
        self.ccd700_collimator_CB.currentIndexChanged.connect(self.ccd700_collimator_changed)
        self.oes_collimator_CB.currentIndexChanged.connect(self.oes_collimator_changed)

        self.ccd700_object_CB.currentIndexChanged.connect(self.ccd700_object_changed)
        self.ccd700_object_changed(-1)

        self.oes_object_CB.currentIndexChanged.connect(self.oes_object_changed)
        self.oes_object_changed(-1)

        self.ccd700_exposure_time_CHB.setChecked(False)
        self.ccd700_exposure_time_CHB.stateChanged.connect(lambda: self.off_CHB_changed("ccd700_exposure_time"))
        self.off_CHB_changed("ccd700_exposure_time")

        self.ccd700_count_of_pulses_CHB.setChecked(True)
        self.ccd700_count_of_pulses_CHB.stateChanged.connect(lambda: self.off_CHB_changed("ccd700_count_of_pulses"))
        self.off_CHB_changed("ccd700_count_of_pulses")

        self.oes_exposure_time_CHB.setChecked(False)
        self.oes_exposure_time_CHB.stateChanged.connect(lambda: self.off_CHB_changed("oes_exposure_time"))
        self.off_CHB_changed("oes_exposure_time")

        self.oes_count_of_pulses_CHB.setChecked(True)
        self.oes_count_of_pulses_CHB.stateChanged.connect(lambda: self.off_CHB_changed("oes_count_of_pulses"))
        self.off_CHB_changed("oes_count_of_pulses")

        self.ccd700_update_exposure_length_BT.clicked.connect(lambda: self.ccd_update_exposure_length_clicked("ccd700"))
        self.oes_update_exposure_length_BT.clicked.connect(lambda: self.ccd_update_exposure_length_clicked("oes"))

        self.load_target_from_stel_BT.clicked.connect(self.load_target_from_stel_clicked)
        self.get_prepared_coordinates_BT.clicked.connect(self.get_prepared_coordinates_clicked)

        self.ccd700_grating_angle_LE.returnPressed.connect(self.set_ccd700_grating_angle_from_LE)

        self.coordinates_status = CoordinatesStatus.UNKNOWN

        self.send_coordinates_BT.clicked.connect(self.send_coordinates_clicked)
        self.go_ra_dec_BT.clicked.connect(self.go_ra_dec_clicked)
        self.ccd700_load_setup_BT.clicked.connect(self.ccd700_load_setup_clicked)
        self.ccd700_spectrograph_set_BT.clicked.connect(self.ccd700_spectrograph_set_clicked)

        self.ccd700_start_BT.clicked.connect(self.ccd700_start_clicked)
        self.ccd700_readout_BT.clicked.connect(self.ccd700_readout_clicked)
        self.oes_start_BT.clicked.connect(self.oes_start_clicked)
        self.oes_readout_BT.clicked.connect(self.oes_readout_clicked)
        self.telescope_set_corrections_BT.clicked.connect(self.telescope_set_corrections_clicked)
        self.telescope_dome_set_relative_BT.clicked.connect(self.telescope_dome_set_relative_clicked)
        self.telescope_dome_set_absolute_BT.clicked.connect(self.telescope_dome_set_absolute_clicked)

        self.dome_auto_BT.clicked.connect(lambda: self.run_ascol_cmd("DOAM"))
        self.dome_stop_BT.clicked.connect(lambda: self.run_ascol_cmd("DOST"))
        self.dome_calibration_BT.clicked.connect(lambda: self.run_ascol_cmd("DOCA"))
        self.slit_close_BT.clicked.connect(lambda: self.run_ascol_cmd("DOSO 0"))
        self.slit_open_BT.clicked.connect(lambda: self.run_ascol_cmd("DOSO 1"))
        self.tracking_off_BT.clicked.connect(lambda: self.run_ascol_cmd("TSGM 0"))
        self.tracking_on_BT.clicked.connect(lambda: self.run_ascol_cmd("TSGM 1"))
        self.telescope_park_BT.clicked.connect(lambda: self.run_ascol_cmd("TEPA"))
        self.telescope_on_BT.clicked.connect(lambda: self.run_ascol_cmd("TEON 1"))
        self.telescope_off_BT.clicked.connect(lambda: self.run_ascol_cmd("TEON 0"))
        self.telescope_stop_BT.clicked.connect(lambda: self.run_ascol_cmd("TEST"))
        self.mirror_flap_open_BT.clicked.connect(lambda: self.run_ascol_cmd("FMOP 1"))
        self.mirror_flap_close_BT.clicked.connect(lambda: self.run_ascol_cmd("FMOP 0"))

        self.progress_callback_dict = {
            "telescope_read": self.progress_telescope_read_fn,
            "telescope_command": self.progress_telescope_command_fn,
            "spectrograph_read": self.progress_spectrograph_read_fn,
            "spectrograph_command": self.progress_spectrograph_command_fn,
            "ccd700_read": self.progress_ccd700_read_fn,
            "ccd700_command": self.progress_ccd700_control_fn,
            "oes_read": self.progress_oes_read_fn,
            "oes_command": self.progress_oes_control_fn,
            "target_select": self.progress_target_select_fn,
        }

        self.threadpool = QThreadPool()
        self.threadpool.setMaxThreadCount(20)

        print("Multithreading with maximum %d threads" % self.threadpool.maxThreadCount())

        self.refresh_statusbar(ClientStatus.IDLE)

        self.exit.clear()
        self.stop_event.clear()
        self.thread_exit["read"].clear()

        kwargs = {
            "name": "telescope_read",
            "gui": self,
            "command": None,
            "thread_exit": self.thread_exit["read"],
        }

        self.telescope_thread = TelescopeThread(self)
        telescope_worker = Worker(self.telescope_thread.run, **kwargs)

        kwargs = {
            "name": "spectrograph_read",
            "gui": self,
            "command": None,
            "thread_exit": self.thread_exit["read"],
        }

        self.spectrograph_thread = SpectrographThread(self)
        spectrograph_worker = Worker(self.spectrograph_thread.run, **kwargs)

        kwargs = {
            "name": "ccd700_read",
            "gui": self,
            "command": None,
            "thread_exit": self.thread_exit["read"],
        }

        self.ccd700_thread = CCDThread(self, "ccd700")
        ccd700_worker = Worker(self.ccd700_thread.run, **kwargs)

        kwargs = {
            "name": "oes_read",
            "gui": self,
            "command": None,
            "thread_exit": self.thread_exit["read"],
        }

        self.oes_thread = CCDThread(self, "oes")
        oes_worker = Worker(self.oes_thread.run, **kwargs)

        self.threadpool.start(telescope_worker)
        self.threadpool.start(spectrograph_worker)
        self.threadpool.start(ccd700_worker)
        self.threadpool.start(oes_worker)

        self.ccd_command_thread = {
            "oes": None,
            "ccd700": None,
        }

        self.resize(1024, 1024)
        self.show()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(OBSERVE_CLIENT_CFG)

        self.grating_angles = {}
        for section in rcp.sections():
            prefix = "ccd700_grating_angle_"
            if section.startswith(prefix):
                name = section[len(prefix):]
                self.grating_angles[name] = dict(rcp.items(section))

        self.logger.info("grating_angles = %s" % self.grating_angles)

        self.cfg = {
            "telescope": {},
            "spectrograph": {},
            "ccd700": {},
            "ccd700_setup": {},
            "oes": {},
            "oes_setup": {},
            "observer": {},
            "pattern": {},
        }

        rcp = configparser.ConfigParser()
        rcp.read(CCD_CONTROL_CFG)

        callbacks = {
            "host": rcp.get,
            "port": rcp.getint,
        }
        self.run_cfg_callbacks("telescope", callbacks)
        self.run_cfg_callbacks("spectrograph", callbacks)
        self.run_cfg_callbacks("ccd700", callbacks)
        self.run_cfg_callbacks("oes", callbacks)

        setup_callbacks = {
            "zero_exptime": rcp.get,
            "dark_exptime": rcp.get,
            "flat_exptime": rcp.get,
            "comp_exptime": rcp.get,
            "target_exptime": rcp.get,
        }
        self.run_cfg_callbacks("oes_setup", setup_callbacks)
        self.run_cfg_callbacks("ccd700_setup", setup_callbacks)

        observer_callbacks = {
            "latitude": rcp.getfloat,
            "longitude": rcp.getfloat,
            "elevation": rcp.getfloat,
        }
        self.run_cfg_callbacks("observer", observer_callbacks)

        pattern_callbacks = {
            "ra": rcp.get,
            "dec": rcp.get,
            "fits_header": rcp.get,
            "time_h_m_s": rcp.get,
            "time_m_s": rcp.get,
            "time_s": rcp.get,
            "count_of_pulses": rcp.get,
        }
        self.run_cfg_callbacks("pattern", pattern_callbacks)

        for section in self.cfg:
            for key in self.cfg[section]:
                if key == "password":
                    self.logger.info("cfg.%s.%s = ****" % (section, key))
                else:
                    self.logger.info("cfg.%s.%s = %s" % (section, key, self.cfg[section][key]))

    def get_restrictions(self, filename):
        data = np.loadtxt(filename)
        x, y = zip(*data)

        return [x, y]

    def run_cfg_callbacks(self, section, callbacks):
        for key in callbacks:
            self.cfg[section][key] = callbacks[key](section, key)

    def grating_pos2angle(self, value):
        value = float(value)
        angle = Angle((-0.00487106 * value + 61.7024) * u.degree)

        return angle.to_string(sep=":", fields=2)

    def progress_spectrograph_read_fn(self, data):
        # DBG
        #print(data)

        # 1. Dichroická zrcátka
        # 2. Spektrální filtr
        # 3. Maska kolimátoru
        # 4. Ostření 700
        # 5. Ostření 1400/400
        # 6. Překlápění hvězda kalibrace
        # 7. Překlápění Coudé/Oes
        # 8. Flat field
        # 9. Srovnávací spektrum
        #10. Závěrka expozimetru
        #11. Závěrka kamery 700
        #12. Závěrka kamery 1400/400
        #13. Mřížka úhel
        #14. Expozimetr
        #15. Štěrbinová kamera
        #16. Korekční deska 700
        #17. Korekční deska 1400/400
        #18. CCD kamera
        #19. Rezerva
        #20. Rezerva
        #21. Maska kolimátoru oes
        #22. Ostření oes
        #23. Závěrka expozimetr oes
        #24. Expozimetr oes
        #25. Rezerva
        #26. Jodová baňka
        #27. Štěrbinová kamera Coudé napájení
        #28. Štěrbinová kamera Oes napájení
        items = data["GLST"].split()
        ccd700_grating_angle= self.grating_pos2angle(data["SPGP_13"])
        ccd700_em_count_of_pulses = float(data["SPCE_14"])
        oes_em_count_of_pulses = float(data["SPCE_24"])

        ccd700_collimator = "unknown"
        value = items[2]
        if value in self.SPECTROGRAPH_COLLIMATOR2STR:
            ccd700_collimator = self.SPECTROGRAPH_COLLIMATOR2STR[value]

        oes_collimator = "unknown"
        value = items[20]
        if value in self.SPECTROGRAPH_COLLIMATOR2STR:
            oes_collimator = self.SPECTROGRAPH_COLLIMATOR2STR[value]

        ccd700_spectral_filter = "unknown"
        value = items[1]
        if value in self.CCD700_SPECTRAL_FILTER2STR:
            ccd700_spectral_filter = self.CCD700_SPECTRAL_FILTER2STR[value]

        ccd700_grating_angle_state = "unknown"
        value = items[12]
        if value in self.CCD700_GRATING_ANGLE_STATE2STR:
            ccd700_grating_angle_state = self.CCD700_GRATING_ANGLE_STATE2STR[value]

        self.set_label_text(self.ccd700_spectral_filter_LB, ccd700_spectral_filter)
        self.set_label_text(self.ccd700_grating_angle_LB, ccd700_grating_angle)
        self.set_label_text(self.ccd700_grating_angle_state_LB, ccd700_grating_angle_state)
        self.set_label_text(self.ccd700_collimator_LB, ccd700_collimator)
        self.set_label_text(self.oes_collimator_LB, oes_collimator)
        self.set_label_text(self.ccd700_em_count_of_pulses_LB, str(ccd700_em_count_of_pulses))
        self.set_label_text(self.oes_em_count_of_pulses_LB, str(oes_em_count_of_pulses))

        self.refresh_exposure_meter_info(
            self.ccd700_count_of_pulses_CHB.isChecked(),
            self.ccd700_status_LB.text(),
            self.ccd700_object_CB.currentText(),
            self.ccd700_exposure_meter_PB,
            self.ccd700_actual_count_of_pulses_str,
            ccd700_em_count_of_pulses)

        self.refresh_exposure_meter_info(
            self.oes_count_of_pulses_CHB.isChecked(),
            self.oes_status_LB.text(),
            self.oes_object_CB.currentText(),
            self.oes_exposure_meter_PB,
            self.oes_actual_count_of_pulses_str,
            oes_em_count_of_pulses)

    def refresh_exposure_meter_info(self, disabled, status, obj, progress_bar, required_count_of_pulses_str, count_of_pulses):
        if disabled:
            return

        if status == "expose" and obj == "target":
            progress_bar.setEnabled(True)

            try:
                required_count_of_pulses_mc = float(required_count_of_pulses_str)
                count_of_pulses_mc = count_of_pulses / 1000000.0
                msg = "%s from %s Mcounts" % (count_of_pulses_mc, required_count_of_pulses_str)
                percent = (count_of_pulses_mc / required_count_of_pulses_mc) * 100
                if percent > 100:
                    percent = 100
                    msg = "success"

                progress_bar.setValue(int(percent))
                progress_bar.setFormat(msg)
            except:
                self.logger.exception("refresh_exposure_meter_info() failed")
        else:
            progress_bar.setEnabled(False)
            progress_bar.setValue(100)
            progress_bar.setFormat("")

    def log_command_result(self, data):
        dt_str = datetime.now().strftime("%H:%M:%S")

        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#0000FF">%(command)s => %(result)s</font><br>' % data)

    def progress_spectrograph_command_fn(self, data):
        self.log_command_result(data)

    def progress_telescope_command_fn(self, data):
        self.log_command_result(data)

    def progress_ccd_read(self, data, args):
        self.set_label_text(args["status_LB"], data["state"])
        self.set_label_text(args["file_fits_LB"], data["filename"])
        self.set_label_text(args["actual_temp_LB"], "%.1f" % data["ccd_temp"])

        if data["state"] == "ready":
            args["progress_bar"].setValue(100)
            args["progress_bar"].setFormat("")

            for widget in args["widgets"]:
                widget.setEnabled(True)
        else:
            for widget in args["widgets"]:
                widget.setEnabled(False)

            if data["full_time"] <= 0:
                percent = 100
            else:
                percent = (data["elapsed_time"] / data["full_time"]) * 100

            remained_time = data["full_time"] - data["elapsed_time"]
            data["remained_time_str"] = str(timedelta(seconds=remained_time))
            args["progress_bar"].setValue(int(percent))
            args["progress_bar"].setFormat("%(remained_time_str)s (%(expose_number)i/%(expose_count)i)" % data)

    def progress_ccd700_read_fn(self, data):
        args = {
            "status_LB": self.ccd700_status_LB,
            "file_fits_LB": self.ccd700_file_fits_LB,
            "actual_temp_LB": self.ccd700_actual_temp_LB,
            "progress_bar": self.ccd700_PB,
            "widgets": self.ccd700_widgets,
        }

        self.progress_ccd_read(data, args)

    def progress_oes_read_fn(self, data):
        args = {
            "status_LB": self.oes_status_LB,
            "file_fits_LB": self.oes_file_fits_LB,
            "actual_temp_LB": self.oes_actual_temp_LB,
            "progress_bar": self.oes_PB,
            "widgets": self.oes_widgets,
        }

        self.progress_ccd_read(data, args)

    def progress_ccd700_control_fn(self, data):
        self.log_command_result(data)

    def progress_oes_control_fn(self, data):
        self.log_command_result(data)

    def progress_target_select_fn(self, targets):
        if self.exit.is_set():
            return

        dialog = SelectTargetDialog(targets, parent=self)

        if not dialog.exec_():
            return

        idx = dialog.get_selected_target_idx()

        if idx == -1:
            return

        target = targets[idx]

        self.star_name_LE.setText(target["object"])
        self.ra_LE.setText(target["ra"])
        self.dec_LE.setText(target["de"])

    def set_label_text(self, label, text, color="#CCFFCC", tooltip=""):
        if text in ["unknown", "OFF", "UNCALIBRATED", "LOCKED", "CLOSE", "OPEN LEFT", "OPEN RIGHT", "TIMEOUT", "MOVING"]:
            color = "#FFCCCC"
        elif text in ["expose"]:
            color = "#FFFF33"
        elif text in ["readout"]:
            color = "#CCCCFF"

        label.setText(text)
        label.setStyleSheet("background-color: %s;" % color)
        label.setToolTip(tooltip)

    # TODO: rozpracovano
    def process_bits(self, bits):

        status_bits2str = {
            0: ["UNCALIBRATED", "CALIBRATED", self.telescope_ha_calibration_LB],   # Hodinová osa je zkalibrovaná
            1: ["UNCALIBRATED", "CALIBRATED", self.telescope_da_calibration_LB],   # Deklinační osa je zkalibrovaná
            2: ["UNCALIBRATED", "CALIBRATED", self.telescope_dome_calibration_LB], # Kopule je zkalibrovaná
            3: ["UNCALIBRATED", "CALIBRATED", None], # Ostření je zkalibrované
        }

        status_bits_name = {
            4: "Abberation",             # Zapnuta korekce aberace
            5: "Precesion and nutation", # Zapnuta korekce precese a nutace
            6: "Refraction",             # Zapnuta korekce refrakce
            7: "Error model",            # Zapnuta korekce chybového modelu dalekohledu
            8: "Guide mode",             # Zapnutý guide režim
        }

        corrections = "ON"
        corrections_tooltip = []

        for shift in range(9):
            value = (bits >> shift) & 1
            if shift >= 4:
                value_str = "ON"
                if value == 0:
                    corrections = "OFF"
                    value_str = "OFF"
                corrections_tooltip.append("%s = %s" % (status_bits_name[shift], value_str))
                continue

            label = status_bits2str[shift][2]
            if label is not None:
                self.set_label_text(label, status_bits2str[shift][value])

        self.set_label_text(self.telescope_corrections_LB, corrections)
        self.telescope_corrections_LB.setToolTip("\n".join(corrections_tooltip))

    def progress_telescope_read_fn(self, data):
        # {
        #     'ut': '2022-04-30 07:27:02',
        #     'glst': '0 1 0 0 0 1 4 4 4 33271 0 0 0 0',
        #     'trrd': '165819.520 +445827.20 0',
        #     'trhd': '89.9903 45.0012',
        #     'trgv': '-2.1 -24.5',
        #     'trus': '0.0000 0.0000',
        #     'dopo': '359.89',
        #     'trcs': '1',
        #     'fopo': '0.00',
        #     'tsra': '212547.02 364002.58 0',
        #     'object': 'unknown'
        # }

        try:
            ra, dec, position = data["tsra"].split(" ")

            self.prepared_star["object"] = data["object"]
            self.prepared_star["ra"] = ra
            self.prepared_star["dec"] = dec
            self.prepared_star["position"] = position
        except:
            # TODO: informovat uzivatele
            self.logger.exception("parse tsra failed")

        try:
            glst = data["glst"].split(" ")
        except:
            # TODO: informovat uzivatele
            self.logger.exception("parse glst failed")

        try:
            ra_correction, dec_correction = data["trgv"].split(" ")
            self.set_label_text(self.telescope_ra_correction_LB, ra_correction)
            self.set_label_text(self.telescope_dec_correction_LB, dec_correction)
            self.set_label_text(self.telescope_correction_model_LB, data["trcs"])
        except:
            # TODO: informovat uzivatele
            self.logger.exception("parse trgv failed")

        idx2key = [
            "oil", "telescope", "ha", "da", "focus", "dome", "slit", "tube_flap", "mirror_flap",
            "bits_state", "bits_err1", "bits_err2", "bits_err3", "bits_err4",
        ]

        global_state2str = {
            "oil": ["OFF", "START1", "START2", "START3", "ON", "OFF_DELAY"],

            "telescope": ["INIT", "OFF", "OFF_WAIT", "STOP", "TRACK", "OFF_REQ",
                          "SS_CLU1", "SS_SLEW", "SS_DECC2", "SS_CLU2", "SS_DECC3",
                          "SS_CLU3", "ST_DECC1", "ST_CLU1", "ST_SLEW", "ST_DECC2",
                          "ST_CLU2", "ST_DECC3", "ST_CLU3"],

            "ha": ["STOP", "POSITION", "CA_CLU1", "CA_FAST", "CA_FASTBR", "CA_CLU2",
                   "CA_SLOW", "MO_BR", "MO_CLU1", "MO_FAST", "MO_FASTBR", "MO_CLU2",
                   "MO_SLOW", "MO_SLOWEST"],

            "da": ["STOP", "POSITION", "CA_CLU1", "CA_FAST", "CA_FASTBR", "CA_CLU2",
                   "CA_SLOW", "MO_BR", "MO_CLU1", "MO_FAST", "MO_FASTBR", "MO_CLU2",
                   "MO_SLOW", "MO_SLOWEST", "CENM_SLOWBR", "CENM_CLU3", "CENM_CEN",
                   "CENM_BR", "CENM_CLU4", "CENA_SLOWBR", "CENA_CLU3", "CENA_CEN",
                   "CENA_BR", "CENA_CLU4"],

            "focus": ["OFF", "STOP", "PLUS", "MINUS", "SLEW", "CAL1", "CAL2"],

            "dome": ["OFF", "STOP", "PLUS", "MINUS", "SLEW_PLUS", "SLEW_MINUS",
                     "AUTO_STOP", "AUTO_PLUS", "AUTO_MINUS", "CALIBRATION"],

            "slit": ["UNDEF", "OPENING", "CLOSING", "OPEN", "CLOSE"],
            "tube_flap": ["UNDEF", "OPENING", "CLOSING", "OPEN", "CLOSE"],
            "mirror_flap": ["UNDEF", "OPENING", "CLOSING", "OPEN", "CLOSE"],
        }

        idx = -1
        for value in glst:
            idx += 1
            if idx >= len(idx2key):
                break

            value = int(value)
            key = idx2key[idx]

            print(key, value)

            if key.startswith("bits_") and key in ["bits_state"]:
                self.process_bits(value)
                continue

            if key not in global_state2str or key in ["focus"]:
                continue

            try:
                value = global_state2str[key][value]
            except:
                traceback.print_exc()
                self.logger.exception("global_state2str failed")
                value = "UNKNOWN"

            widget_name = "telescope_%s_LB" % key
            if not hasattr(self, widget_name):
                self.logger.error("QLabel '%s' not found" % widget_name)
                continue

            label = getattr(self, widget_name)
            self.set_label_text(label, value)

        for key in data:
            value = data[key]

            if key == "ut":
                dt = datetime.strptime(value, "%Y-%m-%d %H:%M:%S")
                value = dt.strftime("%Y-%m-%d %H:%M:%S")
                observing_time = Time(dt, location=self.earth_location)
                alt_az = AltAz(location=self.earth_location, obstime=observing_time)
                lst = observing_time.sidereal_time("mean")
                self.set_label_text(self.lst_LB, lst.to_string(sep=":"))
                self.set_label_text(self.utc_LB, value)
                self.set_label_text(self.telescope_dome_position_LB, data["dopo"])

                ra_raw, dec_raw, position_raw = data["trrd"].split(" ")

                ra = self.ra_pattern.search(ra_raw)
                if ra:
                    ra = ra.groupdict()
                    ra = "%(dd)sh%(mm)sm%(ss)ss" % ra
                else:
                    ra = None

                dec = self.dec_pattern.search(dec_raw)
                if dec:
                    dec = dec.groupdict()
                    dec = "%(dd)sd%(mm)sm%(ss)ss" % dec
                else:
                    dec = None

                if ra is not None and dec is not None:
                    coord = SkyCoord(ra, dec, frame="icrs", equinox="J2000")
                    c = coord.transform_to(alt_az)

                    # TODO: pri zmene RA, DEC prekreslit okamzite, pokud jiz dalekohled najel na souradnice
                    diff = (datetime.now(tz=timezone.utc) - self.matplotlib_dt).total_seconds()
                    if diff > 5:
                        self.matplotlib_dt = datetime.now(tz=timezone.utc)
                        try:
                            self.star_matplotlib_canvas.plot_star(coord, observing_time, self.observer)
                            self.limits_matplotlib_canvas.plot_star(coord, observing_time, self.observer)
                        except:
                            traceback.print_exc()
                            self.logger.exception("plot_star() failed")

                    self.set_label_text(self.altitude_LB, c.alt.to_string(sep=":"))
                    self.set_label_text(self.azimuth_LB, c.az.to_string(sep=":"))
                    self.set_label_text(self.airmass_LB, "%.2f" % c.secz)
                    self.set_label_text(self.coordinates_ra_LB, coord.ra.to_string(unit=u.hour, sep=":"))
                    self.set_label_text(self.coordinates_dec_LB, coord.dec.to_string(sep=":"))

                    if self.coordinates_status == CoordinatesStatus.PREPARED:
                        self.coordinates_ra_LB.setStyleSheet("background-color: #CCCCCC;")
                        self.coordinates_dec_LB.setStyleSheet("background-color: #CCCCCC;")
                    else:
                        self.coordinates_ra_LB.setStyleSheet("background-color: #CCFFCC;")
                        self.coordinates_dec_LB.setStyleSheet("background-color: #CCFFCC;")

                    ha = lst - coord.ra
                    self.set_label_text(self.ha_LB, ha.to_string(sep=":"))

        # TODO
        return

        # 0
        # Hodinová osa je zkalibrovaná
        # 1
        # Deklinační osa je zkalibrovaná
        # 2
        # Kopule je zkalibrovaná
        # 3
        # Ostření je zkalibrované
        # 4
        # Zapnuta korekce aberace
        # 5
        # Zapnuta korekce precese a nutace
        # 6
        # Zapnuta korekce refrakce
        # 7
        # Zapnuta korekce chybového modelu dalekohledu
        # 8
        # Zapnutý guide režim
        # 15
        # Pavouk 1 (stará funkce) = 0 / Pavouk 2 = 1

        status_bits2str = {
              0: ["OFF", "ON", self.remote_mode_LB],                       #  0 System is in REMOTE mode
              1: ["OFF", "ON", self.control_voltage_LB],                   #  1 Control voltage is turned on
              2: ["UNCALIBRATED", "CALIBRATED", self.ha_calibration_LB],   #  2 HA axis is calibrated
              3: ["UNCALIBRATED", "CALIBRATED", self.da_calibration_LB],   #  3 DEC axis is calibrated
              4: ["UNCALIBRATED", "CALIBRATED", self.dome_calibration_LB], #  4 Dome is calibrated
              5: ["OFF", "ON", self.correction_refraction_state_LB],       #  5 Correction of refraction is turned on
              6: ["OFF", "ON", self.correction_model_state_LB],            #  6 Correction model function is turned on
              7: ["OFF", "ON", self.tracking_LB],                          #  7 Guide mode is turned on
              8: ["", "MOVE", None],                                       #  8 Focusing is in move
              9: ["OFF", "ON", self.dome_lamp_LB],                         #  9 Dome light is on
              10: ["OFF", "ON", self.vent_tube_state_LB],                  # 10 Vent on tube is on
              11: ["LOCKED", "UNLOCKED", self.ha_lock_LB],                 # 11 HA axis unlocked
              12: ["LOCKED", "UNLOCKED", self.da_lock_LB],                 # 12 DEC axis unlocked
              13: ["OFF", "ON", self.dome_camera_power_LB],                # 13 Dome camera is on
        }

        for shift in range(14):
            value = (data.global_state.status_bits >> shift) & 1
            label = status_bits2str[shift][2]
            if label is not None:
                self.set_label_text(label, status_bits2str[shift][value])

        error_bits2str = {
            0: "Error of motor or regulation of HA",
            1: "Error of motor or regulation of DA",
            2: "Negative restriction of HA",
            3: "Positive restriction of HA",
            4: "Negative restriction of DA",
            5: "Positive restriction of DA",
            6: "general error",
            7: "telescope error",
            8: "dome or slit error",
            9: "focus error",
            10: "meteo error",
        }

        errors = []
        error_flag = False
        for shift in range(11):
            value = (data.global_state.error_bits >> shift) & 1
            if value:
                error_flag = True
                error_msg = error_bits2str[shift]
                errors.append(error_msg)
                self.set_label_text(self.error_msg_LB, error_msg, color="#FFCCCC")

        if error_flag:
            self.error_msg_LB.setToolTip("\n".join(errors))
        else:
            self.set_label_text(self.error_msg_LB, "Alright")

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

        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#FF0000">%s</font><br>' % value)

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

    def refresh_statusbar(self, value, msg=""):
        dt_str = datetime.now().strftime("%H:%M:%S")
        name = value.name
        self.status = value

        self.statusbar.showMessage("Status: %s %s %s" % (name, dt_str, msg))
        self.statusbar.setStyleSheet("background-color: %s;" % self.status_color[name])

    def create_ccd_command_thread(self, name, command):
        self.refresh_statusbar(ClientStatus.RUNNING)

        self.exit.clear()
        self.stop_event.clear()
        self.thread_exit["command"].clear()

        kwargs = {
            "name": "%s_command" % name,
            "gui": self,
            "command": command,
            "thread_exit": self.thread_exit["command"],
        }

        dt_str = datetime.now().strftime("%H:%M:%S")
        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#000000">%s: %s</font><br>' % (name, command))
        self.logger.info("create_ccd_command_thread(%s, %s)" % (name, command))

        # TODO: zvazit vyuziti queue
        self.ccd_command_thread[name] = CCDThread(self, name)

        ccd_worker = Worker(self.ccd_command_thread[name].run, **kwargs)

        self.threadpool.start(ccd_worker)

    def create_spectrograph_command_thread(self, command):
        self.refresh_statusbar(ClientStatus.RUNNING)

        self.exit.clear()
        self.stop_event.clear()
        self.thread_exit["command"].clear()

        kwargs = {
            "name": "spectrograph_command",
            "gui": self,
            "command": command,
            "thread_exit": self.thread_exit["command"],
        }

        dt_str = datetime.now().strftime("%H:%M:%S")
        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#000000">Spectrograph: %s</font><br>' % command)
        self.logger.info("create_spectrograph_command_thread(%s)" % command)

        self.spectrograph_thread = SpectrographThread(self)
        spectrograph_worker = Worker(self.spectrograph_thread.run, **kwargs)

        self.threadpool.start(spectrograph_worker)

    def create_ascol_command_thread(self, command):
        self.refresh_statusbar(ClientStatus.RUNNING)

        self.exit.clear()
        self.stop_event.clear()
        self.thread_exit["command"].clear()

        kwargs = {
            "name": "telescope_command",
            "gui": self,
            "command": command,
            "thread_exit": self.thread_exit["command"],
        }

        dt_str = datetime.now().strftime("%H:%M:%S")
        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#000000">Telescope: %s</font><br>' % command)
        self.logger.info("create_ascol_command_thread(%s)" % command)

        telescope_thread = TelescopeThread(self)
        telescope_worker = Worker(telescope_thread.run, **kwargs) # Any other args, kwargs are passed to the run function

        self.threadpool.start(telescope_worker)

    def run_ascol_clicked(self):
        command = [self.ascol_LE.text()]
        self.create_ascol_command_thread(command)

    def show_msg(self, msg, category="info"):
        category2icon = {
            "question": QMessageBox.Question,
            "info": QMessageBox.Information,
            "warning": QMessageBox.Warning,
            "error": QMessageBox.Critical,
        }

        icon = QMessageBox.NoIcon
        if category in category2icon:
            icon = category2icon[category]

        msg_box = QMessageBox()
        msg_box.setIcon(icon)
        msg_box.setText(msg)
        msg_box.setWindowTitle("CCD control: %s" % category)
        msg_box.setStandardButtons(QMessageBox.Ok)
        msg_box.exec()

    def set_ccd700_grating_angle(self, grating_angle_str):
        try:
            grating_angle = Angle("%s degrees" % grating_angle_str)
        except:
            exctype, value = sys.exc_info()[:2]
            self.show_msg("Bad grating angle '%s'\n\n%s" % (grating_angle_str, value), "error")
            return

        increments = int(np.round(-205.294 * grating_angle.value + 12667.1))

        if increments < 0 or increments > 0xFFFF:
            self.show_msg("Bad grating angle '%s'\n\nOut of range" % grating_angle_str, "error")
            return

        command = []
        command.append("SPAP 13 %i" % increments)

        self.create_spectrograph_command_thread(command)

    def set_ccd700_grating_angle_from_LE(self):
        grating_angle_LE = self.sender()

        self.set_ccd700_grating_angle(grating_angle_LE.text())

    def set_collimator(self, idx, text):
        value = "0"
        for key in self.SPECTROGRAPH_COLLIMATOR2STR:
            if self.SPECTROGRAPH_COLLIMATOR2STR[key] == text:
                value = key
                break

        command = []
        command.append("SPCH %i %s" % (idx, value))

        self.create_spectrograph_command_thread(command)

    def off_CHB_changed(self, name):
        check_box = getattr(self, "%s_CHB" % name)
        line_edit = getattr(self, "%s_LE" % name)

        if check_box.isChecked():
            line_edit.setEnabled(False)
            line_edit.setText("")
        else:
            line_edit.setEnabled(True)

    def set_enabled_widgets(self, widgets, value):
        for widget in widgets:
            widget.setEnabled(value)

    def ccd_object_changed(self, args):
        if args["current_obj"] == "target":
            self.set_enabled_widgets(args["target_widgets_enabled"], True)
            args["target_LE"].setText("")
        else:
            args["count_of_pulses_CHB"].setChecked(True)
            args["exposure_time_CHB"].setChecked(False)
            args["target_LE"].setText(args["current_obj"])
            self.set_enabled_widgets(args["target_widgets_enabled"], False)

        exptime_str = self.history[args["ccd"]]["%s_exptime" % args["current_obj"]]

        args["exposure_time_LE"].setText(exptime_str)

    def ccd700_object_changed(self, idx):
        args = {
            "ccd": "ccd700",
            "current_obj": self.ccd700_object_CB.currentText(),
            "target_LE": self.ccd700_target_LE,
            "count_of_pulses_CHB": self.ccd700_count_of_pulses_CHB,
            "exposure_time_CHB": self.ccd700_exposure_time_CHB,
            "target_LE": self.ccd700_target_LE,
            "exposure_time_LE": self.ccd700_exposure_time_LE,

            "target_widgets_enabled": [
                self.ccd700_count_of_pulses_LE,
                self.ccd700_count_of_pulses_CHB,
                self.ccd700_target_LE,
                self.ccd700_exposure_time_CHB,
            ]
        }

        self.ccd_object_changed(args)

    def oes_object_changed(self, idx):
        args = {
            "ccd": "oes",
            "current_obj": self.oes_object_CB.currentText(),
            "target_LE": self.oes_target_LE,
            "count_of_pulses_CHB": self.oes_count_of_pulses_CHB,
            "exposure_time_CHB": self.oes_exposure_time_CHB,
            "target_LE": self.oes_target_LE,
            "exposure_time_LE": self.oes_exposure_time_LE,

            "target_widgets_enabled": [
                self.oes_count_of_pulses_LE,
                self.oes_count_of_pulses_CHB,
                self.oes_target_LE,
                self.oes_exposure_time_CHB,
            ]
        }

        self.ccd_object_changed(args)

    def oes_collimator_changed(self, idx):
        text = self.oes_collimator_CB.currentText()
        self.set_collimator(21, text)

    def ccd700_collimator_changed(self, idx):
        text = self.ccd700_collimator_CB.currentText()
        self.set_collimator(3, text)

    def ccd700_spectral_filter_changed(self, idx):
        command = []
        command.append("SPCH 2 %s" % self.ccd700_spectral_filter_CB.currentText())

        self.create_spectrograph_command_thread(command)

    def telescope_correction_model_changed(self, idx):
        command = []
        command.append("TSCM %s" % self.telescope_correction_model_CB.currentText())

        self.create_ascol_command_thread(command)

    def process_exposure_meter(self, args):
        if args["exposure_meter_disabled"]:
            expmeter = -1
        else:
            match = self.count_of_pulses_pattern.search(args["count_of_pulses_str"])
            if match:
                result = match.groups()
                expmeter = int(float(result[0]) * 1000000)
            else:
                self.show_msg("Bad count of pulses '%s'" % args["count_of_pulses_str"], "error")
                return [-1, False]

        return [expmeter, True]

    def process_exposure_time(self, args):
        if args["exptime_disabled"]:
            exptime = -1
        else:
            for pattern in [self.time_h_m_s_pattern, self.time_m_s_pattern, self.time_s_pattern]:
                match = pattern.search(args["exposure_time_str"])
                if match:
                    result = match.groups()
                    result_len = len(result)

                    hours = 0 if result_len < 3 else int(result[-3])
                    minutes = 0 if result_len < 2 else int(result[-2])
                    seconds = int(result[-1])

                    exptime = (3600 * hours) + (60 * minutes) + seconds
                    break
            else:
                self.show_msg("Bad exposure time '%s'" % args["exposure_time_str"], "error")
                return [-1, False]

        return [exptime, True]

    def ccd_start_expose(self, args):
        expmeter, result = self.process_exposure_meter(args)
        if not result:
            return

        exptime, result = self.process_exposure_time(args)
        if not result:
            return

        for name, value in [("Observers", self.observers_LE.text()), ("Target", args["target"])]:
            match = self.fits_header_pattern.search(value)
            if not match:
                msg = ("%s format must be:\n\n"
                       "- ASCII text characters ranging from hexadecimal 20 to 7E\n"
                       "- max length is 68 characters\n"
                       "- min length is 1 character")

                self.show_msg(msg % name, "error")
                return

        setup = {
            "READOUT_SPEED": args["readout_speed"],
            "GAIN": "default",
            "PATH": args["data_path"],
            "ARCHIVE": "1",
        }

        image_type = args["obj"]

        if image_type in ["target", "dark"]:
            self.history["oes"]["%s_exptime" % image_type] = args["exposure_time_str"]

        if image_type == "target":
            image_type = "object"

        keys = {
            "IMAGETYP": image_type,
            "OBJECT": args["target"],
            "OBSERVER": self.observers_LE.text(),
        }

        if args["ccd"] == "oes":
            keys["SPECFILT"] = self.oes_spectral_filter_CB.currentText().split()[0]
            keys["SLITHEIG"] = self.oes_slit_height_CB.currentText().split()[0]

        command = []
        for key in setup:
            command.append(["set", key, setup[key]])

        for key in keys:
            command.append(["set_key", key, keys[key], ""])

        # exptime in seconds (-1 = CCD_EXPTIME_MAX), expmeter in counts (-1 = OFF)
        command.append(["start", exptime, args["expcount"], expmeter])

        self.create_ccd_command_thread(args["ccd"], command)

    def ccd_start_expose_wrapper(self, args):
        try:
            self.ccd_start_expose(args)
        except:
            self.logger.exception("ccd_start_expose(%s) failed" % args)
            exctype, value = sys.exc_info()[:2]
            self.show_msg("%s: %s" % (exctype, value), "error")

    def ccd700_start_clicked(self):
        self.ccd700_start_BT.setEnabled(False)

        args = {
            "ccd": "ccd700",
            "expcount": self.ccd700_count_repeat_SB.value(),
            "count_of_pulses_str": self.ccd700_count_of_pulses_LE.text(),
            "exposure_meter_disabled": self.ccd700_count_of_pulses_CHB.isChecked(),
            "exptime_disabled": self.ccd700_exposure_time_CHB.isChecked(),
            "exposure_time_str": self.ccd700_exposure_time_LE.text(),
            "target": self.ccd700_target_LE.text(),
            "readout_speed": self.ccd700_readout_speed_CB.currentText(),
            "data_path": self.ccd700_data_path_CB.currentText(),
            "obj": self.ccd700_object_CB.currentText(),
        }

        self.ccd700_actual_count_of_pulses_str = self.ccd700_count_of_pulses_LE.text()

        self.ccd_start_expose_wrapper(args)

    def ccd700_readout_clicked(self):
        self.create_ccd_command_thread("ccd700", [["readout"]])

    def oes_start_clicked(self):
        self.oes_start_BT.setEnabled(False)

        args = {
            "ccd": "oes",
            "expcount": self.oes_count_repeat_SB.value(),
            "count_of_pulses_str": self.oes_count_of_pulses_LE.text(),
            "exposure_meter_disabled": self.oes_count_of_pulses_CHB.isChecked(),
            "exptime_disabled": self.oes_exposure_time_CHB.isChecked(),
            "exposure_time_str": self.oes_exposure_time_LE.text(),
            "target": self.oes_target_LE.text(),
            "readout_speed": self.oes_readout_speed_CB.currentText(),
            "data_path": self.oes_data_path_CB.currentText(),
            "obj": self.oes_object_CB.currentText(),
        }

        self.oes_actual_count_of_pulses_str = self.oes_count_of_pulses_LE.text()

        self.ccd_start_expose_wrapper(args)

    def oes_readout_clicked(self):
        self.create_ccd_command_thread("oes", [["readout"]])

    def get_prepared_coordinates_clicked(self):
        self.star_name_LE.setText(self.prepared_star["object"])
        self.ra_LE.setText(self.prepared_star["ra"])
        self.dec_LE.setText(self.prepared_star["dec"])

        # 0 = east, 1 = west
        if self.prepared_star["position"] in ["0", "1"]:
            self.telescope_position_CB.setCurrentIndex(int(self.prepared_star["position"]))

    def load_target_from_stel_clicked(self):
        # WARNING: Pokud by byla instance objektu SelectTargetThread pouze jako
        # lokalni promena teto metody, tak by se ve vetsine pripadu tato instance
        # uvolnila z pameti drive nez by byla moznost zavolat jeji metody, ktere
        # jsou navazany na jednotlive udalosti.
        self.select_target_thread = SelectTargetThread(self)

        kwargs = {
            "name": "target_select",
            "gui": self,
            "target": self.star_name_LE.text(),
            "thread_exit": self.thread_exit["target"],
        }

        target_worker = Worker(self.select_target_thread.run, **kwargs) # Any other args, kwargs are passed to the run function

        self.threadpool.start(target_worker)

    def ccd_update_exposure_length_clicked(self, ccd):
        if ccd == "ccd700":
            args = {
                "exposure_meter_disabled": self.ccd700_count_of_pulses_CHB.isChecked(),
                "exptime_disabled": self.ccd700_exposure_time_CHB.isChecked(),
                "count_of_pulses_str": self.ccd700_count_of_pulses_LE.text(),
                "exposure_time_str": self.ccd700_exposure_time_LE.text(),
            }

            self.ccd700_actual_count_of_pulses_str = self.ccd700_count_of_pulses_LE.text()
        else:
            args = {
                "exposure_meter_disabled": self.oes_count_of_pulses_CHB.isChecked(),
                "exptime_disabled": self.oes_exposure_time_CHB.isChecked(),
                "count_of_pulses_str": self.oes_count_of_pulses_LE.text(),
                "exposure_time_str": self.oes_exposure_time_LE.text(),
            }

            self.oes_actual_count_of_pulses_str = self.ccd700_count_of_pulses_LE.text()

        expmeter, result = self.process_exposure_meter(args)
        if not result:
            return

        exptime, result = self.process_exposure_time(args)
        if not result:
            return

        command = []
        command.append(["time_update", exptime])
        command.append(["meter_update", expmeter])

        self.create_ccd_command_thread(ccd, command)

    def ccd700_spectrograph_set_clicked(self):
        if self.actual_grating_angle is None:
            return

        self.set_ccd700_grating_angle(self.actual_grating_angle["grating_angle"])
        self.ccd700_spectral_filter_CB.setCurrentText(self.actual_grating_angle["spectral_filter"])
        self.ccd700_spectral_filter_changed(0)

        self.history["ccd700"]["flat_exptime"] = self.actual_grating_angle["flat"]
        self.history["ccd700"]["comp_exptime"] = self.actual_grating_angle["comp"]

        current_obj = self.ccd700_object_CB.currentText()
        if current_obj in ["flat", "comp"]:
            self.ccd700_exposure_time_LE.setText(self.actual_grating_angle[current_obj])

        self.ccd700_spectrograph_set_LB.setStyleSheet("background-color: #CCFFCC;")

    def ccd700_load_setup_clicked(self):
        dialog = SelectGratingAngleDialog(self.grating_angles, parent=self)

        if not dialog.exec_():
            self.actual_grating_angle = None
            return

        self.actual_grating_angle = dialog.get_selected_grating_angle()

        # {'name': 'gaia', 'grating_angle': '35:47', 'spectral_filter': '3', 'range': '8400-8870', 'flat': '15', 'comp': '100'}
        setup = "%(name)s, GA: %(grating_angle)s, SF: %(spectral_filter)s" % self.actual_grating_angle
        tooltip = "range: %(range)s, flat: %(flat)s, comp: %(comp)s" % self.actual_grating_angle

        self.set_label_text(self.ccd700_spectrograph_set_LB, setup, tooltip=tooltip)

        self.ccd700_spectrograph_set_LB.setStyleSheet("background-color: #CCCCCC;")

    def send_coordinates_clicked(self):
        self.coordinates_status = CoordinatesStatus.PREPARED
        ra = self.ra_LE.text()
        dec = self.dec_LE.text()

        position = 0 # east
        if self.telescope_position_CB.currentText() == "west":
            position = 1

        command = []
        command.append("TSRA %s %s %i" % (ra, dec, position))

        self.create_ascol_command_thread(command)

    def go_ra_dec_clicked(self):
        self.coordinates_status = CoordinatesStatus.MOVING
        command = []
        command.append("TGRA")

        self.create_ascol_command_thread(command)

    def user_offsets_absolute_clicked(self):
        ra = self.user_offsets_ra_DSB.value()
        dec = self.user_offsets_dec_DSB.value()

        command = []
        command.append("TSUA %.1f %.1f" % (ra, dec))

        self.create_ascol_command_thread(command)

    def autoguider_offsets_absolute_clicked(self):
        ra = self.autoguider_offsets_ra_DSB.value()
        dec = self.autoguider_offsets_dec_DSB.value()

        command = []
        command.append("TSGA %.1f %.1f" % (ra, dec))

        self.create_ascol_command_thread(command)

    def dome_position_clicked(self):
        command = []
        command.append("DOSA %.2f" % self.dome_position_DSB.value())
        command.append("DOGA")

        self.create_ascol_command_thread(command)

    def focus_position_clicked(self):
        command = []
        command.append("FOSA %.3f" % self.focus_position_DSB.value())
        command.append("FOGA")

        self.create_ascol_command_thread(command)

    def set_focus_relative_position(self, direction):
        command = []
        command.append("FOSR %s%.3f" % (direction, self.focus_relative_position_DSB.value()))
        command.append("FOGR")

        self.create_ascol_command_thread(command)

    def run_ascol_cmd(self, cmd):
        self.create_ascol_command_thread([cmd])

    def telescope_set_corrections_clicked(self):
        command = []
        command.append("TSGV %.1f %.1f" % (self.telescope_ra_correction_DSB.value(), self.telescope_dec_correction_DSB.value()))

        self.create_ascol_command_thread(command)

    def telescope_dome_set_relative_clicked(self):
        command = []
        command.append("DOSR %.3f" % self.telescope_dome_set_relative_DSB.value())
        command.append("DOGR")

        self.create_ascol_command_thread(command)

    def telescope_dome_set_absolute_clicked(self):
        command = []
        command.append("DOSA %.3f" % self.telescope_dome_set_absolute_DSB.value())
        command.append("DOGA")

        self.create_ascol_command_thread(command)

    def autoguider_offset(self, action):
        ra_offset = 0.0
        dec_offset = 0.0

        if action == "ra-":
            ra_offset -= self.autoguider_offset_relative_DSB.value()
        elif action == "ra+":
            ra_offset += self.autoguider_offset_relative_DSB.value()
        elif action == "dec-":
            dec_offset -= self.autoguider_offset_relative_DSB.value()
        elif action == "dec+":
            dec_offset += self.autoguider_offset_relative_DSB.value()

        command = []
        command.append("TSGR %.2f %.2f" % (ra_offset, dec_offset))

        self.create_ascol_command_thread(command)

    def go_ha_da_clicked(self):
        ha = self.axes_ha_DSB.value()
        da = self.axes_da_DSB.value()

        command = []
        command.append("TSHA %.4f %.4f" % (ha, da))
        command.append("TGHA")

        self.create_ascol_command_thread(command)

    def closeEvent(self, event):
        self.logger.info("closeEvent(event = %s)" % event)
        self.exit.set()

        all_thread_exit = False

        while not all_thread_exit:
            all_thread_exit = True

            for key in self.thread_exit:
                if self.thread_exit[key].is_set():
                    self.logger.info("Thread '%s' is exited." % key)
                else:
                    all_thread_exit = False

            if not all_thread_exit:
                self.logger.info("Waiting...")
                time.sleep(1)

        self.logger.info("GUI exiting...")

def main():
    app = QApplication([])
    ccd_control_ui = CCDClientUI()

    return app.exec()

if __name__ == '__main__':
    main()
