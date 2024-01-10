#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2020-2023 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# https://www.learnpyqt.com/courses/concurrent-execution/multithreading-pyqt-applications-qthreadpool/
#
# https://github.com/mherrmann/fbs-tutorial
#
#     fbs is the fastest way to create a Python GUI. It solves common pain points
#     such as packaging and deployment. Based on Python and Qt, fbs is a
#     lightweight alternative to Electron.
#
# https://www.learnpyqt.com/tutorials/qtableview-modelviews-numpy-pandas/
#
# Events and signals in PyQt5
# https://zetcode.com/gui/pyqt5/eventssignals/
#
# https://www.learnpyqt.com/
#

import os
import sys
import re
import time
import json
import requests
import xmlrpc.client
import configparser
import traceback
import xmlrpc.client
import threading
import logging

from operator import methodcaller
from enum import Enum
from datetime import datetime
from logging.handlers import RotatingFileHandler

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

OBSERVE_CLIENT_UI = "%s/../share/observe_client.ui" % SCRIPT_PATH

OBSERVE_CLIENT_CFG = "%s/../etc/observe_client.cfg" % SCRIPT_PATH
FIBER_CONTROL_CLIENT_CFG = "%s/../etc/fiber_control_client.cfg" % SCRIPT_PATH

OBSERVE_CLIENT_LOG = "%s/../log/observe_client_%%s.log" % SCRIPT_PATH

def init_logger(logger, filename):
    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - %(message)s")

    # DBG
    #formatter = logging.Formatter(
    #    ("%(asctime)s - %(name)s[%(process)d] - %(levelname)s - "
    #     "%(filename)s:%(lineno)s - %(funcName)s() - %(message)s - "))

    fh = RotatingFileHandler(filename, maxBytes=10485760, backupCount=10)
    fh.setLevel(logging.INFO)
    #fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)

    logger.setLevel(logging.INFO)
    #logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

class ClientStatus(Enum):

    IDLE = 0
    RUNNING = 1
    SUCCESS = 2
    FAILED = 3

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
    progress = pyqtSignal(str, dict)

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
            self.thread_exit.set()
            self.signals.finished.emit()  # Done

class SelectTargetDialog(QDialog):

    def __init__(self, targets, parent=None):
        super().__init__(parent=parent)

        self.setWindowTitle("ObserveClient: Select Target")

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

class SelectTargetThread:

    def __init__(self, gui, row):
        self.gui = gui
        self.row = row

    def run(self, progress_callback, target):

        params = {
            "sname": target,
            "out": "json",
        }

        auth = ("public", "db2m")

        response = requests.get("https://stelweb.asu.cas.cz/stars/startab.html", params=params, auth=auth)

        if response.status_code != 200:
            raise Exception("ERROR: response.status_code = %i" % response.status_code)

        stars = response.json()

        return stars

    def result(self, targets):
        print("SelectTargetThread.result")

        if self.gui.exit.is_set():
            return

        dialog = SelectTargetDialog(targets, parent=self.gui)

        if not dialog.exec_():
            return

        idx = dialog.get_selected_target_idx()

        if idx == -1:
            return

        target_LE = self.gui.observe_TW.cellWidget(self.row, self.gui.columns["Target"])
        ra_LE = self.gui.observe_TW.cellWidget(self.row, self.gui.columns["RA"])
        dec_LE = self.gui.observe_TW.cellWidget(self.row, self.gui.columns["DEC"])
        target = targets[idx]

        target_LE.setText(target["object"])
        ra_LE.setText(target["ra"])
        dec_LE.setText(target["de"])

        print(self.row)

    def finished(self):
        print("SelectTargetThread.finished")

        if self.gui.exit.is_set():
            print("SelectTargetThread COMPLETE! Exiting...")
            return

        if self.gui.status != ClientStatus.FAILED:
            self.gui.refresh_statusbar(ClientStatus.SUCCESS)

        #set_enable_widgets(True)
        print("SelectTargetThread COMPLETE!")

    def progress(self, category, data):
        print("SelectTargetThread.progress")

        if self.gui.exit.is_set():
            return

        #if category in progress_callback_dict:
        #    progress_callback_dict[category](data)

    def error(self, error):
        print("SelectTargetThread.error")

        if self.gui.exit.is_set():
            return

        exctype, value, format_exc = error

        self.gui.refresh_statusbar(ClientStatus.FAILED, "%s %s" % (exctype, value))

# TODO: sloucit kod s SelectTargetDialog
class SelectSchedulesDialog(QDialog):

    def __init__(self, schedules, parent=None):
        super().__init__(parent=parent)

        self.setWindowTitle("ObserveClient: Select Schedule")

        buttons = QDialogButtonBox.Ok | QDialogButtonBox.Cancel

        self.button_box = QDialogButtonBox(buttons)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self.columns = {
            'ID': 0,
            'Designation': 1,
            'Count of stars': 2,
        }

        conv = {
            'idsch': 'ID',
            'designation': 'Designation',
            'stars_num': 'Count of stars',
        }

        schedules_len = len(schedules)

        self.targets_TW = QTableWidget()
        self.targets_TW.setSelectionMode(QAbstractItemView.SingleSelection)
        self.targets_TW.setColumnCount(len(self.columns))
        self.targets_TW.setRowCount(schedules_len)
        self.targets_TW.setHorizontalHeaderLabels(self.columns.keys())
        self.targets_TW.setSelectionBehavior(QTableWidget.SelectRows)
        self.targets_TW.setEditTriggers(QTableWidget.NoEditTriggers)

        for row, target in enumerate(schedules):
            for key in target:
                column_name = conv[key]
                column = self.columns[column_name]
                value = target[key]

                self.targets_TW.setItem(row, column, QTableWidgetItem(value))

        if schedules_len > 0:
            self.targets_TW.setCurrentCell(0, 0)

        self.layout = QVBoxLayout()
        self.layout.addWidget(self.targets_TW)
        self.layout.addWidget(self.button_box)
        self.setLayout(self.layout)

        # TODO: vyresit lepe
        self.resize(768, 1080)

    def get_selected_schedule_idx(self):
        current_row = self.targets_TW.currentRow()

        return current_row

# TODO: sloucit kod s SelectTargetThread
class LoadFromStarsThread:

    def __init__(self, gui, action, idsch=-1):
        self.gui = gui
        self.action = action
        self.idsch = idsch

        self.params = {
            "out": "json",
        }

        if action == "schedule":
            self.params["idsch"] = idsch
            self.url = "https://stelweb.asu.cas.cz/stars/schtab.html"
        else:
            self.url = "https://stelweb.asu.cas.cz/stars/schlist.html"

    def make_calibration(self, name, json_item):

        grating_angle = ""
        spectral_filter = 0
        exptime = int(json_item["caltime"])

        spectral_range = json_item["spectral_range"]

        if spectral_range in self.gui.grating_angles:
            values = self.gui.grating_angles[spectral_range]
            grating_angle =  "%s %s" % (spectral_range, values["grating_angle"])
            spectral_filter = values["spectral_filter"]

            if name in ["flat", "comp"] and exptime == 0:
                exptime = int(values[name])

        calibration = {
            "Object": name,
            "Target": name,
            "Count repeat": 1,
            "Exposure length": exptime,
            "RA": "",
            "DEC": "",
            "TelPos": "east",
            "Tracking": "on",
            "Sleep": 1,
            "CamPos": int(json_item["fiber"]),
            "FocPos": 0,
            "Filter": "same",
            "Grating angle": "",
            "Spectral filter": 0,
            "Count of pulses": 0.0,
            "Iodine cell": json_item["iodinecell"],
            "Enable": True,
        }

        return [calibration, grating_angle, spectral_filter]

    def stars2batch(self, json):
        batch = []

        params = {
            "out": "json",
        }

        url = "https://stelweb.asu.cas.cz/stars/startab.html"

        # Stars JSON:
        #
        #     "name":"bet Ori flat", (zero|dark|flat|comp)
        #     "v_mag":"",
        #     "note":"",
        #     "ra":"",
        #     "de":"",
        #     "exptime":"10",
        #     "caltime":"0",
        #     "iodinecell":false,
        #     "count_repeat":"5",
        #     "count_of_pulses":null,
        #     "fiber":1,
        #     "spectral_range":"hbeta",
        #     "ga_sf":null,
        #     "start":"17:59",
        #     "ha":"",
        #     "alt":""
        for json_item in json:
            calibration_name = ""
            calibration, grating_angle, spectral_filter = self.make_calibration("zero", json_item)
            params["sname"] = json_item["name"]

            if json_item["name"].find(" ") != -1:
                sname_tmp, calibration_name_tmp = json_item["name"].rsplit(" ", 1)

                if calibration_name_tmp in ["zero", "dark", "flat", "comp"]:
                    calibration_name = calibration_name_tmp
                    calibration, grating_angle, spectral_filter = self.make_calibration(calibration_name, json_item)
                    params["sname"] = sname_tmp

            for star_item in self.stars_get(url, params):
                if star_item["object"].lower() == params["sname"].lower():
                    star = star_item
                    break
            else:
                raise Exception("Star '%s' not found" % params["sname"])

            count_of_pulses = 0.0
            value = json_item["count_of_pulses"]
            if value is not None:
                count_of_pulses = float(value)

            count_repeat = 1
            value = json_item["count_repeat"]
            if value is not None:
                count_repeat = int(value)

            batch.append({
                "Object": "target",
                "Target": star["object"],
                "Count repeat": count_repeat,
                "Exposure length": int(json_item["exptime"]),
                "RA": star["ra"],
                "DEC": star["de"],
                "TelPos": "east",
                "Tracking": "on",
                "Sleep": 1,
                "CamPos": int(json_item["fiber"]),
                "FocPos": 0,
                "Filter": "same",
                "Grating angle": grating_angle,
                "Spectral filter": spectral_filter,
                "Count of pulses": count_of_pulses,
                "Iodine cell": json_item["iodinecell"],
                "Enable": True,
            })

            if calibration_name:
                batch.append(calibration)

        return batch

    def stars_get(self, url, params):
        auth = ("public", "db2m")

        response = requests.get(url, params=params, auth=auth)

        if response.status_code != 200:
            raise Exception("ERROR: response.status_code = %i" % response.status_code)

        return response.json()

    def run(self, progress_callback):
        json = self.stars_get(self.url, self.params)

        if self.action == "schedule":
            json = self.stars2batch(json)

        return json

    def result(self, json):
        if self.gui.exit.is_set():
            return

        if self.action == "schedule":
            self.gui.load_batch(json)
        else:
            dialog = SelectSchedulesDialog(json, parent=self.gui)

            if not dialog.exec_():
                return

            idx = dialog.get_selected_schedule_idx()

            if idx == -1:
                return

            self.gui.create_thread_load_from_stars("schedule", json[idx]["idsch"])

    def finished(self):
        if self.gui.exit.is_set():
            return

        if self.gui.status != ClientStatus.FAILED:
            self.gui.refresh_statusbar(ClientStatus.SUCCESS)

    def progress(self, category, data):
        if self.gui.exit.is_set():
            return

    def error(self, error):
        if self.gui.exit.is_set():
            return

        exctype, value, format_exc = error

        self.gui.refresh_statusbar(ClientStatus.FAILED, "%s %s" % (exctype, value))

class BatchThread:

    def __init__(self, gui):
        self.gui = gui
        self.cfg = gui.cfg
        self.is_stop = gui.is_stop
        self.star_is_ready_event = gui.star_is_ready_event

    # TODO: ccd700 setup spectrograph (grating angle)
    def run(self, progress_callback, instrument, batch, observers, data_path):
        self.progress_callback = progress_callback # TODO: přesunout do konstruktoru

        process_name = "batch"
        self.logger = logging.getLogger("observe_client_%s" % process_name)
        init_logger(self.logger, OBSERVE_CLIENT_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.proxy = {}
        self.proxy["telescope"] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["telescoped"])
        self.proxy["toptec"] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["toptec"])
        self.proxy["quido"] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["quido"])
        self.proxy["expose"] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg[instrument])
        self.proxy["spectrograph"] = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.cfg["spectrographd"])

        for item in batch:
            data = { "row": item["row"] }
            progress_callback.emit("set_current_row", data)

            if self.is_stop():
                return "Exit."

            if not item["Enable"]:
                continue

            item.update({"instrument": instrument})

            telescope_setup = self.set_telescope(item)
            spectrograph_setup = self.set_spectrograph(item)
            toptec_setup = self.set_toptec(item)
            relays_setup = self.set_relays(item)

            self.wait_toptec(toptec_setup)
            self.wait_spectrograph(spectrograph_setup)

            if telescope_setup is not None:
                self.wait_telescope(telescope_setup)

            progress_callback.emit("ready", item)
            time.sleep(item["Sleep"])

            if telescope_setup is not None:
                self.logger.info("Star is ready")
                self.star_is_ready_event.clear()
                progress_callback.emit("star_is_ready", telescope_setup)

                while not self.is_stop() and not self.star_is_ready_event.is_set():
                    time.sleep(0.1)

            item["cfg"] = self.cfg[instrument]
            item["observers"] = observers
            item["data_path"] = self.cfg[instrument][data_path.lower()]
            for idx in range(item["Count repeat"]):
                item.update({"exposure_number": idx+1})

                if instrument == "photometry":
                    self.expose_photometry_start(item)
                else:
                    self.expose_start(item)

                while not self.is_stop():
                    if instrument == "photometry":
                        result = self.proxy["expose"].get_status()
                        if result["status"] == 1: # ready
                            self.logger.info("Expose success")
                            break
                    else:
                        result = self.proxy["expose"].expose_info()
                        if result["state"] == "ready":
                            self.logger.info("Expose success")
                            break

                    progress_callback.emit("expose", item)
                    time.sleep(1)

                if self.is_stop() or instrument != "photometry":
                    break

        return "Done."

    def set_toptec(self, args):
        self.progress_callback.emit("msg", {"msg": "toptec setup"})

        focus_position = args["FocPos"]
        cameras_position = args["CamPos"]

        if focus_position != 0:
            result = self.proxy["toptec"].toptec_set_focus_position(focus_position)
            self.logger.info("toptec_set_focus_position(%i) => %s" % (focus_position, result))

        if cameras_position != 0:
            result = self.proxy["toptec"].toptec_set_camera_position(cameras_position)
            self.logger.info("toptec_set_camera_position(%i) => %s" % (cameras_position, result))

        values = {
            "focus_position": focus_position,
            "cameras_position": cameras_position,
        }

        return values

    def wait_toptec(self, setup):

        for counter in range(60):
            if self.is_stop():
                break

            values = self.proxy["toptec"].toptec_get_values()
            success = True
            for key in setup:
                diff = abs(values[key] - setup[key])
                if setup[key] != 0 and diff > 1:
                    success = False

            self.progress_callback.emit("toptec", values)

            if success:
                self.logger.info("required = %s\nactual = %s\ntoptec setup success" % (setup, values))
                break

            time.sleep(1)
        else:
            msg = "Toptec setup timeout"
            self.logger.error(msg)
            raise Exception(msg)

    def set_relays(self, args):
        self.progress_callback.emit("msg", {"msg": "relays setup"})

        # 1. comp
        # 2. flat
        values = {
            1: 0,
            2: 0,
        }

        if args["Object"] == "comp":
            values[1] = 1
        elif args["Object"] == "flat":
            values[2] = 1

        for relay_number in values:
            result = self.proxy["quido"].quido_set_relay(relay_number, values[relay_number])
            self.logger.info("quido_set_relay(%i, %i) => %s (object = %s)" % (relay_number, values[relay_number], result, args))

            for counter in range(5):
                time.sleep(1)
                relays = self.proxy["quido"].quido_get_relays()

                if relays[relay_number-1] == values[relay_number]:
                    self.logger.info("required = %s\nactual = %s\nrelays setup success" % (values, relays))
                    break
            else:
                msg = "Relays setup timeout"
                self.logger.error(msg)
                raise Exception(msg)

        data = {"msg": 2}
        self.progress_callback.emit("relays", data)

        return values

    def set_telescope(self, args):
        #'Tracking': 'on',

        self.progress_callback.emit("msg", {"msg": "telescope setup"})

        if not args["RA"] or not args["DEC"]:
            return None

        if args["TelPos"] == "east":
            position = 0
        else:
            position = 1

        params = {
            "ra": args["RA"],
            "dec": args["DEC"],
            "position": position,
            "object": args["Target"],
        }

        result = self.proxy["telescope"].telescope_set_coordinates(params)
        self.logger.info("telescope_set_coordinates(%s) => %s" % (params, result))

        # TODO: zkontrolovat zda-li se jiz hodnoty prednastavily
        time.sleep(2)

        telescope_cmd = "TGRA 1"
        result = self.proxy["telescope"].telescope_execute(telescope_cmd)
        self.logger.info("telescope_execute(%s) => %s" % (telescope_cmd, result))

        if result != "1":
            raise Exception("TGRA result = '%s'" % result)

        return params

    @staticmethod
    def grating_angle2increments(value):
        grating_angle_pattern = re.compile("^([0-9]{1,3}):([0-9]{1,2}(\.[0-9]{0,3})?)$")

        value_match = grating_angle_pattern.search(value)

        if (not value_match):
            raise Exception("Unexpected value '%s'. Grating angle must be [DD]D:[M]M[.MMM]." % value)

        degrees_str, minutes_str, decimal_str = value_match.groups()

        degrees = int(degrees_str)
        minutes = float(minutes_str)

        degrees += (minutes / 60.0)

        increments = round(-205.294 * degrees + 12667.1)

        return increments

    def set_spectrograph(self, args):
        self.progress_callback.emit("msg", {"msg": "spectrograph setup"})

        if args["instrument"] == "ccd700":
            ga_increments = None
            spectral_filter = None

            values = args["Grating angle"].split(" ")
            if values[0]:
                # values is [ga_name, ga_value] or [ga_value]
                ga_increments = self.grating_angle2increments(values[-1])
                cmd = "SPAP 13 %i" % ga_increments
                result = self.proxy["spectrograph"].spectrograph_execute(cmd)
                self.logger.info("spectrograph_execute(%s) => '%s'" % (cmd, result))

                if result != "1":
                    raise Exception("'%s' result = '%s'" % (cmd, result))

            value = args["Spectral filter"]
            if value != "same":
                spectral_filter = int(value)
                cmd = "SPCH 2 %i" % spectral_filter
                result = self.proxy["spectrograph"].spectrograph_execute(cmd)
                self.logger.info("spectrograph_execute(%s) => '%s'" % (cmd, result))

                if result != "1":
                    raise Exception("'%s' result = '%s'" % (cmd, result))

            return {"grating_angle": ga_increments, "spectral_filter": spectral_filter}
        elif args["instrument"] == "oes":
            # WARNING: Hodnoty pro nastaveni jsou 1 = remove, 2 = insert, ale stav se vycita 1 = inserted, 2 = removed
            value = 2 if args["Iodine cell"] else 1
            cmd = "SPCH 26 %i" % value
            result = self.proxy["spectrograph"].spectrograph_execute(cmd)
            self.logger.info("spectrograph_execute(%s) => '%s'" % (cmd, result))

            if result != "1":
                raise Exception("'%s' result = '%s'" % (cmd, result))

            value = 1 if args["Iodine cell"] else 2
            return {"iodine_cell": value}
        elif args["instrument"] == "photometry":
            # TODO
            return {}
        else:
            raise Exception("Unknown instrument '%s'" % args["instrument"])

    def wait_spectrograph(self, setup):

        if not setup:
            return

        for counter in range(60):
            if self.is_stop():
                break

            actual_values = {}
            success = True
            for key in setup:
                actual_values[key] = ""

                if setup[key] is None:
                    continue

                # TODO: potvrdit akceptovatelný rozdíl
                if key == "grating_angle":
                    value = int(self.proxy["spectrograph"].spectrograph_execute("SPGP 13"))
                    actual_values[key] = value
                    diff = abs(value - setup[key])
                    if diff > 1:
                        success = False
                elif key == "spectral_filter":
                    value = int(self.proxy["spectrograph"].spectrograph_execute("SPGS 2"))
                    actual_values[key] = value
                    if value != setup[key]:
                        success = False
                elif key == "iodine_cell":
                    value = int(self.proxy["spectrograph"].spectrograph_execute("SPGS 26"))
                    actual_values[key] = value
                    if value != setup[key]:
                        success = False

            self.progress_callback.emit("spectrograph", actual_values)

            if success:
                break

            time.sleep(1)
        else:
            raise Exception("Spectrograph setup timeout")

    @staticmethod
    def hms_str2s(values):
        h, m, s = values

        sign = 1
        if (h[0] == "-"):
            sign = -1

        h = abs(float(h)) * 3600
        m = float(m) * 60
        s = float(s)

        result = ((h + m + s) * sign)

        return result

    # TODO: rozpracovano
    def wait_telescope(self, setup):

        tsra_accuracy = 10.0 # arc seconds
        pattern_angle = re.compile("^([+-]{0,1}[0-9]{2})([0-9]{2})([0-9]{2}\.[0-9]{2,3})$")

        for counter in range(300):
            if self.is_stop():
                break

            time.sleep(1)
            result = self.proxy["telescope"].telescope_info()

            # DBG
            print("telescope = %s" % result)

            glst = result["glst"].split(" ")

            # tracking == 4
            if glst[1] != "4":
                continue

            # auto_stop == 6
            if glst[5] != "6":
                continue

            ra, dec, position = result["trrd"].split(" ")

            values = []
            for value in [setup["ra"], setup["dec"], ra, dec]:
                value_match = pattern_angle.search(value)

                if (not value_match):
                    raise Exception("Unexpected value '%s'" % value)

                values.append(self.hms_str2s(value_match.groups()))

            ra_expected, dec_expected, ra_actual, dec_actual = values

            pos_expected = setup["position"]
            pos_actual = int(position)

            self.progress_callback.emit("telescope", {"RA": ra_actual, "DEC": dec_actual, "TelPos": pos_actual})

            # DBG
            print("ra_expected =", ra_expected)
            print("ra_actual =", ra_actual)
            print("dec_expected =", dec_expected)
            print("dec_actual =", dec_actual)
            print("pos_expected =", pos_expected)
            print("pos_actual =", pos_actual)

            if (abs(ra_expected - ra_actual) > tsra_accuracy):
                continue
            if (abs(dec_expected - dec_actual) > tsra_accuracy):
                continue
            if (pos_expected != pos_actual):
                continue

            # telescope setup success
            break
        else:
            raise Exception("Telescope setup timeout")

    def expose_start(self, args):
        # expose_set
        setup = [
            ["READOUT_SPEED", args["cfg"]["readout_speed"]],
            ["GAIN", args["cfg"]["gain"]],
            ["PATH", args["data_path"]],
            ["ARCHIVE", args["cfg"]["archive"]],
        ]

        for item in setup:
            result = self.proxy["expose"].expose_set(*item)
            self.logger.info("expose_set(%s) => %s" % (item, result))

        imagetyp = args["Object"]
        if args["Object"] == "target":
            imagetyp = "object"

        # expose_set_key
        setup_key = [
            ["IMAGETYP", imagetyp, ""],
            ["OBJECT", args["Target"], ""],
            ["OBSERVER", args["observers"], ""],

            # oes
            #["SPECFILT", "", ""],
            #["SLITHEIG", "", ""],
        ]

        for item in setup_key:
            result = self.proxy["expose"].expose_set_key(*item)
            self.logger.info("expose_set_key(%s) => %s" % (item, result))

        exposure_time = args["Exposure length"]
        exposure_count = args["Count repeat"]

        # TODO
        exposure_meter = int(args["Count of pulses"] * 10**6)
        if exposure_meter == 0.0:
            # -1 = disable
            exposure_meter = -1
        else:
            # -1 = exposure time max
            exposure_time = -1

        result = self.proxy["expose"].expose_start(exposure_time, exposure_count, exposure_meter)
        self.logger.info("expose_start(%s, %s, %s) => %s" % (exposure_time, exposure_count, exposure_meter, result))
        time.sleep(1)

    def expose_photometry_start(self, args):
        imagetyp = args["Object"]
        if args["Object"] == "target":
            imagetyp = "object"

        exposure_time = args["Exposure length"]
        exposure_count = 1

        if args["Filter"] != "same":
            self.proxy["expose"].set_filter(int(args["Filter"]))

        self.proxy["expose"].start_exposure(exposure_time, 0, exposure_count, 0, imagetyp, args["Target"], args["observers"])
        # TODO
        self.logger.info("start_exposure()")
        time.sleep(1)

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

        self.resize(1024, 768)

    def get_selected_grating_angle(self):
        row = self.grating_angles_TW.currentRow()
        item = {}

        for key in self.columns:
            table_widget = self.grating_angles_TW.item(row, self.columns[key])
            item[key] = table_widget.text()

        return item

class ObserveObjectComboBox(QComboBox):

    def __init__(self, value, parent, target_LE, ra_LE, dec_LE, exposure_length_SB):
        super(ObserveObjectComboBox, self).__init__()

        self.parent = parent
        self.target_LE = target_LE
        self.ra_LE = ra_LE
        self.dec_LE = dec_LE
        self.exposure_length_SB = exposure_length_SB

        self.items = [
            "zero",
            "dark",
            "flat",
            "comp",
            "target",
        ]

        self.exptime = {
            "flat": {},
            "comp": {},
        }

        index = 0
        current_index = 0
        for item in self.items:
            if item == value:
                current_index = index

            self.addItem(item)
            index += 1

        self.setCurrentIndex(current_index)
        self.set_target_LE(value)

    def set_target_LE(self, obj):
        target = self.target_LE.text()
        enabled = False

        if obj == "target" and target in ["zero", "dark", "flat", "comp"]:
            target = ""
            enabled = True
        elif obj == "target":
            enabled = True
        else:
            target = obj
            self.ra_LE.setText("")
            self.dec_LE.setText("")

        self.target_LE.setEnabled(enabled)
        self.target_LE.setText(target)

        self.set_exposure_length_SB()

    # TODO: zavolat i při změně CCD, aby se načetlo správné nastavení exptime pro flat a comp
    def set_exposure_length_SB(self):
        obj = self.currentText()

        if obj not in ["flat", "comp"]:
            return

        instrument = self.get_instrument()

        if instrument not in self.parent.cfg or instrument != "oes":
            return

        if obj not in self.parent.cfg[instrument]:
            return

        if instrument in self.exptime[obj]:
            value = int(self.exptime[obj][instrument])
        else:
            value = int(self.parent.cfg[instrument][obj])

        self.exposure_length_SB.setValue(value)

    def get_instrument(self):
        return self.parent.instrument_CB.currentText().lower()

    def set_exptime(self, flat, comp):
        instrument = self.get_instrument()

        self.exptime["flat"][instrument] = flat
        self.exptime["comp"][instrument] = comp

        self.set_exposure_length_SB()

class ObserveCountRepeatSpinBox(QSpinBox):

    def __init__(self, value):
        super(ObserveCountRepeatSpinBox, self).__init__()

        self.setMinimum(1)
        self.setMaximum(100)
        self.setValue(value)

class ObserveTargetLineEdit(QLineEdit):

    def __init__(self, value):
        super(ObserveTargetLineEdit, self).__init__()

        self.setText(value)

class ObserveGratingAngleLineEdit(QLineEdit):

    def __init__(self, value, parent, object_CB, spectral_filter_CB):
        super(ObserveGratingAngleLineEdit, self).__init__()

        self.parent = parent
        self.object_CB = object_CB
        self.spectral_filter_CB = spectral_filter_CB
        self.grating_angle = None
        self.tip = "Press ENTER for Grating angle dialog"

        for name in self.parent.grating_angles:
            if name.lower() == value.lower():
                self.grating_angle = self.parent.grating_angles[name]
                self.grating_angle["name"] = name
                break

        if self.grating_angle is None:
            self.setText(value)
            self.setToolTip(self.tip)
        else:
            self.setGratingAngle(self.grating_angle)

    def setGratingAngle(self, value):
        self.grating_angle = value
        self.setText("%(name)s %(grating_angle)s" % value)
        self.object_CB.set_exptime(value["flat"], value["comp"])
        # TODO: ošetřit neplatné hodnoty
        self.spectral_filter_CB.setCurrentIndex(int(value["spectral_filter"]))

        items = []
        for key in value:
            items.append("%s = %s" % (key, value[key]))

        grating_angle_str = ", ".join(items)

        self.setToolTip("%s\n\n%s" % (grating_angle_str, self.tip))

        columns = self.parent.columns
        observe_TW = self.parent.observe_TW
        for row in range(observe_TW.currentRow()+1, observe_TW.rowCount()):
            grating_angle_LE = observe_TW.cellWidget(row, columns["Grating angle"])

            if grating_angle_LE.text():
                break
            else:
                object_CB = observe_TW.cellWidget(row, columns["Object"])
                object_CB.set_exptime(value["flat"], value["comp"])

class ObserveExposureLengthSpinBox(QSpinBox):

    def __init__(self, value):
        super(ObserveExposureLengthSpinBox, self).__init__()

        self.setMinimum(0)
        self.setMaximum(3*3600)
        self.setValue(value)

class ObserveRaLineEdit(QLineEdit):

    def __init__(self, value):
        super(ObserveRaLineEdit, self).__init__()

        self.setText(value)

class ObserveDecLineEdit(QLineEdit):

    def __init__(self, value):
        super(ObserveDecLineEdit, self).__init__()

        self.setText(value)

class ObserveTelPosComboBox(QComboBox):

    def __init__(self, value):
        super(ObserveTelPosComboBox, self).__init__()

        self.items = [
            "east",
            "western",
        ]

        index = 0
        current_index = 0
        for item in self.items:
            if item == value:
                current_index = index

            self.addItem(item)
            index += 1

        self.setCurrentIndex(current_index)

class ObserveTrackingComboBox(QComboBox):

    def __init__(self, value):
        super(ObserveTrackingComboBox, self).__init__()

        self.items = [
            "on",
            "off",
        ]

        index = 0
        current_index = 0
        for item in self.items:
            if item == value:
                current_index = index

            self.addItem(item)
            index += 1

        self.setCurrentIndex(current_index)

class ObserveSleepSpinBox(QSpinBox):

    def __init__(self, value):
        super(ObserveSleepSpinBox, self).__init__()

        self.setMinimum(0)
        self.setMaximum(30)
        self.setValue(value)

class ObserveCountOfPulsesDoubleSpinBox(QDoubleSpinBox):

    def __init__(self, value):
        super(ObserveCountOfPulsesDoubleSpinBox, self).__init__()

        self.setMinimum(0.0)
        self.setMaximum(9999.0)
        self.setValue(value)
        self.setDecimals(2)
        self.setSingleStep(0.1)

        tip = [
            "Exposure meter count in Mcounts",
            "0 = disable",
        ]

        self.setToolTip("\n".join(tip))

    def textFromValue(self, value):
        text = "%.2f" % value

        if value == 0.0:
            text = "disable"

        return text

class ObserveCamPosSpinBox(QSpinBox):

    def __init__(self, value, cfg, parent, object_CB):
        super(ObserveCamPosSpinBox, self).__init__()

        self.cfg = cfg
        self.parent = parent
        self.object_CB = object_CB

        self.setMinimum(0)
        self.setMaximum(15999)

        if value in [0, 1, 2, 3]:
            value = self.valueFromText(value)

        self.setValue(value)

        tip = [
            "0 = same",
            "1 = ccd700 1. fiber, oes fiber, photometric fiber",
            "2 = ccd700 2. fiber",
            "3 = ccd700 both fibers",
        ]

        self.setToolTip("\n".join(tip))

    def textFromValue(self, value):
        text = "%i" % value

        for item in self.cfg:
            if value == self.cfg[item]:
                text = "%s: %i" % (item, value)

        return text

    def valueFromText(self, text):
        instrument = self.parent.instrument_CB.currentText().lower()
        obj = self.object_CB.currentText()

        value = self.getCamPos(text, instrument, obj, self.cfg)

        return value

    def getCamPos(self, text, instrument, obj, cfg):
        value = int(text)
        key = ""

        if obj == "target":
            obj = "object"

        # ccd700_object_1fiber
        # ccd700_object_2fiber
        # ccd700_comp_1fiber
        # ccd700_comp_2fiber
        # ccd700_flat_1fiber
        # ccd700_flat_2fiber
        # ccd700_flat_both_fibers
        if instrument == "ccd700":
            value_str = "%ifiber" % value

            if value == 3:
                value_str = "both_fibers"

            key = "%s_%s_%s" % (instrument.lower(), obj, value_str)
        # photometric_g2
        elif instrument == "photometry" and value == 1:
            key = "photometric_g2"
        # oes_object
        # oes_comp
        # oes_flat
        elif instrument == "oes" and value == 1:
            key = "%s_%s" % (instrument.lower(), obj)

        if key in cfg:
            value = cfg[key]

        return value

class ObserveFocPosSpinBox(QSpinBox):

    def __init__(self, value, cfg):
        super(ObserveFocPosSpinBox, self).__init__()

        self.cfg = cfg

        self.setMinimum(0)
        self.setMaximum(8192)
        self.setValue(value)

        tip = [
            "0 = same",
            "1 = G1",
            "2 = G2",
        ]

        self.setToolTip("\n".join(tip))

    def textFromValue(self, value):
        text = "%i" % value

        for item in self.cfg:
            if value == self.cfg[item]:
                text = "%s: %i" % (item, value)

        return text

    def valueFromText(self, text):
        value = int(text)
        key = "g%i" % value

        if key in self.cfg:
            value = self.cfg[key]

        return value

class ObserveFilterComboBox(QComboBox):

    def __init__(self, value):
        super(ObserveFilterComboBox, self).__init__()

        self.items = [
            "same",
            "0",
            "1",
            "2",
            "3",
            "4",
        ]

        index = 0
        current_index = 0
        for item in self.items:
            if item == value:
                current_index = index

            self.addItem(item)
            index += 1

        self.setCurrentIndex(current_index)

class ObserveSpectralFilterComboBox(QComboBox):

    def __init__(self, value):
        super(ObserveSpectralFilterComboBox, self).__init__()

        self.items = [
            "same",
            "1",
            "2",
            "3",
            "4",
            "5",
        ]

        index = 0
        current_index = 0
        for item in self.items:
            if item == value:
                current_index = index

            self.addItem(item)
            index += 1

        self.setCurrentIndex(current_index)

class ObserveEnableCheckBox(QCheckBox):

    def __init__(self, value):
        super(ObserveEnableCheckBox, self).__init__()

        value = bool(value)

        self.setChecked(value)

class ObserveClientUI(QMainWindow):

    def __init__(self):
        super(ObserveClientUI, self).__init__()

        process_name = "gui"
        self.logger = logging.getLogger("observe_client_%s" % process_name)
        init_logger(self.logger, OBSERVE_CLIENT_LOG % process_name)
        self.logger.info("Starting process '%s'" % process_name)

        self.exit = threading.Event()
        self.stop_event = threading.Event()
        self.star_is_ready_event = threading.Event()
        self.status = ClientStatus.IDLE

        self.thread_exit = {}
        for key in ["instrument", "target", "schedule", "schedule_list"]:
            self.thread_exit[key] = threading.Event()
            self.thread_exit[key].set()

        self.status_color = {
            "IDLE": "#CCCCCC",
            "RUNNING": "#CCFFCC",
            "SUCCESS": "#FFFFFF",
            "FAILED": "#FFCCCC",
        }

        self.iodine_cell_position2str = [
            "STOP",
            "inserted",
            "removed",
            "movement",
            "ALARM",
        ]

        self.load_cfg()

        uic.loadUi(OBSERVE_CLIENT_UI, self)

        self.actionLoad_from_SSH.setEnabled(False)
        self.actionLoad_from_URL.setEnabled(False)
        self.actionExport_to_SSH.setEnabled(False)
        self.actionExport_to_Stars.setEnabled(False)
        self.actionExport_to_URL.setEnabled(False)

        self.widgets = []

        self.instrument_differences = {
            "ccd700": [
                self.instrument_difference_init(self.name01_LB, self.value01_LB, "Count of pulses:"),
                self.instrument_difference_init(self.name02_LB, self.value02_LB, "Grating angle:"),
                self.instrument_difference_init(self.name03_LB, self.value03_LB, "Spectral filter:"),
            ],
            "oes": [
                self.instrument_difference_init(self.name01_LB, self.value01_LB, "Count of pulses:"),
                self.instrument_difference_init(self.name02_LB, self.value02_LB, "Iodine cell:"),
                self.instrument_difference_init(self.name03_LB, self.value03_LB),
            ],
            "photometry": [
                self.instrument_difference_init(self.name01_LB, self.value01_LB, "Filter:"),
                self.instrument_difference_init(self.name02_LB, self.value02_LB),
                self.instrument_difference_init(self.name03_LB, self.value03_LB),
            ],
        }

        self.progress_callback_dict = {
            "ready": self.progress_ready_fn,
            "expose": self.progress_expose_fn,
            "telescope": self.progress_telescope_fn,
            "spectrograph": self.progress_spectrograph_fn,
            "toptec": self.progress_toptec_fn,
            "relays": self.progress_relays_fn,
            "star_is_ready": self.progress_star_is_ready_fn,
            "set_current_row": self.progress_set_current_row_fn,
        }

        self.columns = {
            "Object": 0,
            "Target": 1,
            "Count repeat": 2,
            "Exposure length": 3,
            "RA": 4,
            "DEC": 5,
            "TelPos": 6,
            "Tracking": 7,
            "Sleep": 8,
            "CamPos": 9,
            "FocPos": 10,
            "Filter": 11,
            "Grating angle": 12,
            "Spectral filter": 13,
            "Count of pulses": 14,
            "Iodine cell": 15,
            "Enable": 16,
        }

        self.actual_grating_angle = None

        batch = []
        enable = True
        for idx in range(3):
            item = self.create_item(enable=enable)
            batch.append(item)

        self.observe_TW.setColumnCount(len(self.columns))
        self.observe_TW.setHorizontalHeaderLabels(self.columns.keys())
        self.observe_TW.setSelectionBehavior(QTableWidget.SelectRows)

        self.load_batch(batch)

        self.start_BT.clicked.connect(self.start_clicked)
        self.stop_BT.clicked.connect(self.stop_clicked)
        self.add_row_BT.clicked.connect(self.add_row_clicked)
        self.remove_row_BT.clicked.connect(self.remove_row_clicked)

        self.instrument_CB.setCurrentIndex(1)
        self.instrument_CB.currentTextChanged.connect(self.instrument_load)

        instrument = self.instrument_CB.currentText().lower()
        self.instrument_load(instrument)

        self.observe_TW.setEditTriggers(QTableWidget.NoEditTriggers)

        self.actionLoad_from_file.triggered.connect(self.load_from_file)
        self.actionExport_to_file.triggered.connect(self.export_to_file)

        self.actionLoad_from_Stars.triggered.connect(self.load_from_stars)

        self.threadpool = QThreadPool()
        self.threadpool.setMaxThreadCount(2)

        print("Multithreading with maximum %d threads" % self.threadpool.maxThreadCount())

        self.refresh_statusbar(ClientStatus.IDLE)

        self.resize(1920, 1080)
        #self.resize(3000, 2000)
        self.show()

    def load_cfg(self):
        self.cfg = {}

        rcp = configparser.ConfigParser()
        rcp.read(OBSERVE_CLIENT_CFG)

        for key in ["telescoped", "spectrographd", "ccd700", "oes", "photometry"]:
            self.cfg[key] = dict(rcp.items(key))

        self.grating_angles = {}
        for section in rcp.sections():
            prefix = "ccd700_grating_angle_"
            if section.startswith(prefix):
                name = section[len(prefix):]
                self.grating_angles[name] = dict(rcp.items(section))

        self.logger.info("grating_angles = %s" % self.grating_angles)

        rcp = configparser.ConfigParser()
        rcp.read(FIBER_CONTROL_CLIENT_CFG)

        self.cfg["toptec"] = dict(rcp.items("toptec"))
        self.cfg["focus"] = dict(rcp.items("focus"))
        self.cfg["camera"] = dict(rcp.items("camera"))
        self.cfg["quido"] = dict(rcp.items("quido"))

        for key in self.cfg["focus"]:
            self.cfg["focus"][key] = int(self.cfg["focus"][key])

        for key in self.cfg["camera"]:
            self.cfg["camera"][key.lower()] = int(self.cfg["camera"][key])

        self.logger.info("cfg = %s" % self.cfg)

    def progress_set_current_row_fn(self, data):
        self.observe_TW.setCurrentCell(data["row"], 0)
        self.current_row_LB.setText(str(data["row"] + 1))

    def progress_star_is_ready_fn(self, data):
        msg_box = QMessageBox()
        msg_box.setIcon(QMessageBox.Question)
        msg_box.setText("Is star '%(object)s' ready?\n\nRA = %(ra)s\nDEC = %(dec)s\nposition = %(position)s" % data)
        msg_box.setWindowTitle("ObserveClient: Is star ready?")
        msg_box.setStandardButtons(QMessageBox.Ok)
        msg_box.exec()

        self.star_is_ready_event.set()

    def progress_ready_fn(self, data):
        self.object_LB.setText(data["Object"])
        self.tracking_LB.setText(data["Tracking"])
        self.count_repeat_LB.setText(str(data["Count repeat"]))
        self.exposure_length_LB.setText(str(data["Exposure length"]))
        self.target_LB.setText(data["Target"])

        # TODO
        #self.instrument_differences["photometry"]["filter"]["value_LB"].setText(str(data["Filter"]))

    def progress_expose_fn(self, data):
        # TODO: zobrazovat pocet hotovych expozic
        #self.count_repeat_LB.setText("%i/%i" % (data["exposure_number"], data["Count repeat"]))
        self.count_repeat_LB.setText("%i" % data["Count repeat"])

    def progress_telescope_fn(self, data):
        widgets = {
            "RA": self.ra_LB,
            "DEC": self.dec_LB,
            "TelPos": self.position_LB,
        }

        for key in data:
            widgets[key].setText(str(data[key]))

    def progress_spectrograph_fn(self, data):
        for key in data:
            if key == "grating_angle":
                self.value02_LB.setText(str(data[key]))
            elif key == "spectral_filter":
                self.value03_LB.setText(str(data[key]))
            elif key == "iodine_cell":
                try:
                    iodine_cell_position = self.iodine_cell_position2str[data[key]]
                except:
                    iodine_cell_position = "UNKNOWN"

                self.value02_LB.setText(iodine_cell_position)

    def progress_toptec_fn(self, data):
        self.focus_position_LB.setText(str(data["focus_position"]))
        self.camera_position_LB.setText(str(data["cameras_position"]))

    # TODO
    def progress_relays_fn(self, data):
        pass

    def batch_progress(self, category, data):
        if self.exit.is_set():
            return

        if category == "msg":
            self.refresh_statusbar(ClientStatus.RUNNING, data["msg"])
        elif category in self.progress_callback_dict:
            self.logger.debug("batch_progress(category = %s, data = %s)" % (category, data))
            self.progress_callback_dict[category](data)
            self.refresh_statusbar(ClientStatus.RUNNING, category)

    def batch_result(self, s):
        if self.exit.is_set():
            return

        self.logger.info("batch_result = %s" % s)

    def batch_error(self, error):
        if self.exit.is_set():
            return

        exctype, value, format_exc = error

        self.refresh_statusbar(ClientStatus.FAILED, "%s %s" % (exctype, value))

        #self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        #self.log_TE.textCursor().insertHtml('<font color="#FF0000">error</font><br>')

    def batch_finished(self):

        if self.exit.is_set():
            self.logger.info("BatchThread COMPLETE! Exiting...")
            return

        if self.status != ClientStatus.FAILED:
            self.refresh_statusbar(ClientStatus.SUCCESS)

        self.set_enable_widgets(True)
        self.logger.info("BatchThread COMPLETE!")

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

    def set_enable_widgets(self, value):
        self.observe_TW.setEnabled(value)

        for widget in self.widgets:
            widget.setEnabled(value)

    def create_item(self, enable, obj="zero", target=""):

        if obj != "target":
            target = obj

        item = {
            'Object': obj,
            'Target': target,
            'Count repeat': 1,
            'Exposure length': 0,
            'RA': '',
            'DEC': '',
            'TelPos': 'east',
            'Tracking': 'on',
            'Sleep': 1,
            'CamPos': 0,
            'FocPos': 0,
            'Filter': 'same',

            #'Grating angle': 'HAlfa',
            #'Grating angle': 'halfa',
            #'Grating angle': '30:15',
            'Grating angle': '',

            'Spectral filter': 'same',
            'Count of pulses': 0.0,
            'Iodine cell': False,
            'Enable': enable,
        }

        return item

    def fill_row(self, row, item):
        exposure_length_SB = ObserveExposureLengthSpinBox(item["Exposure length"])
        self.observe_TW.setCellWidget(row, self.columns["Exposure length"], exposure_length_SB)

        target_LE = ObserveTargetLineEdit(item["Target"])
        target_LE.returnPressed.connect(self.select_target)
        self.observe_TW.setCellWidget(row, self.columns["Target"], target_LE)

        ra_LE = ObserveRaLineEdit(item["RA"])
        self.observe_TW.setCellWidget(row, self.columns["RA"], ra_LE)

        dec_LE = ObserveDecLineEdit(item["DEC"])
        self.observe_TW.setCellWidget(row, self.columns["DEC"], dec_LE)

        object_CB = ObserveObjectComboBox(item["Object"], self, target_LE, ra_LE, dec_LE, exposure_length_SB)
        self.observe_TW.setCellWidget(row, self.columns["Object"], object_CB)
        object_CB.currentTextChanged.connect(self.object_changed)

        spinbox = ObserveCountRepeatSpinBox(item["Count repeat"])
        self.observe_TW.setCellWidget(row, self.columns["Count repeat"], spinbox)

        combobox = ObserveTelPosComboBox(item["TelPos"])
        self.observe_TW.setCellWidget(row, self.columns["TelPos"], combobox)

        combobox = ObserveTrackingComboBox(item["Tracking"])
        self.observe_TW.setCellWidget(row, self.columns["Tracking"], combobox)

        spinbox = ObserveSleepSpinBox(item["Sleep"])
        self.observe_TW.setCellWidget(row, self.columns["Sleep"], spinbox)

        spinbox = ObserveCamPosSpinBox(item["CamPos"], self.cfg["camera"], self, object_CB)
        self.observe_TW.setCellWidget(row, self.columns["CamPos"], spinbox)

        spinbox = ObserveFocPosSpinBox(item["FocPos"], self.cfg["focus"])
        self.observe_TW.setCellWidget(row, self.columns["FocPos"], spinbox)

        combobox = ObserveFilterComboBox(item["Filter"])
        self.observe_TW.setCellWidget(row, self.columns["Filter"], combobox)

        spectral_filter_CB = ObserveSpectralFilterComboBox(item["Spectral filter"])
        self.observe_TW.setCellWidget(row, self.columns["Spectral filter"], spectral_filter_CB)

        grating_angle_LE = ObserveGratingAngleLineEdit(item["Grating angle"], self, object_CB, spectral_filter_CB)
        grating_angle_LE.returnPressed.connect(self.select_grating_angle)
        self.observe_TW.setCellWidget(row, self.columns["Grating angle"], grating_angle_LE)

        spinbox = ObserveCountOfPulsesDoubleSpinBox(item["Count of pulses"])
        self.observe_TW.setCellWidget(row, self.columns["Count of pulses"], spinbox)

        checkbox = ObserveEnableCheckBox(item["Iodine cell"])
        self.observe_TW.setCellWidget(row, self.columns["Iodine cell"], checkbox)

        checkbox = ObserveEnableCheckBox(item["Enable"])
        self.observe_TW.setCellWidget(row, self.columns["Enable"], checkbox)

    def load_batch(self, batch):

        self.observe_TW.clearContents()
        self.observe_TW.setRowCount(len(batch))
        row = 0
        for item in batch:
            self.fill_row(row, item)
            row += 1

    def object_changed(self, name):
        object_CB = self.sender()

        object_CB.set_target_LE(name)

    def instrument_difference_init(self, name_LB, value_LB, name=""):
        difference = {
            "name_LB": name_LB,
            "value_LB": value_LB,
            "name": name,
        }

        return difference

    def instrument_load(self, name):
        name = name.lower()
        filter_hide = True
        grating_angle_hide = True
        spectral_filter_hide = True
        count_of_pulses_hide = True
        iodine_cell_hide = True

        if name == "ccd700":
            grating_angle_hide = False
            spectral_filter_hide = False
            count_of_pulses_hide = False
        elif name == "oes":
            count_of_pulses_hide = False
            iodine_cell_hide = False
        elif name == "photometry":
            filter_hide = False

        self.observe_TW.setColumnHidden(self.columns["Filter"], filter_hide)
        self.observe_TW.setColumnHidden(self.columns["Grating angle"], grating_angle_hide)
        self.observe_TW.setColumnHidden(self.columns["Spectral filter"], spectral_filter_hide)
        self.observe_TW.setColumnHidden(self.columns["Count of pulses"], count_of_pulses_hide)
        self.observe_TW.setColumnHidden(self.columns["Iodine cell"], iodine_cell_hide)

        fce_show = methodcaller("show")
        fce_hide = methodcaller("hide")

        for difference in self.instrument_differences[name]:

            difference["name_LB"].setText(difference["name"])
            difference["value_LB"].setText("")

            if difference["name"]:
                fce_show(difference["name_LB"])
                fce_show(difference["value_LB"])
            else:
                fce_hide(difference["name_LB"])
                fce_hide(difference["value_LB"])

        for row in range(0, self.observe_TW.rowCount()):
            object_CB = self.observe_TW.cellWidget(row, self.columns["Object"])
            object_CB.set_exposure_length_SB()

    def get_batch(self, current_row, insert_row_number=False):

        batch = []
        for row in range(current_row, self.observe_TW.rowCount()):

            item = {}
            for key in self.columns:

                widget = self.observe_TW.cellWidget(row, self.columns[key])

                if isinstance(widget, QComboBox):
                    value = widget.currentText()
                elif isinstance(widget, QSpinBox):
                    value = widget.value()
                elif isinstance(widget, QDoubleSpinBox):
                    value = widget.value()
                elif isinstance(widget, QLineEdit):
                    value = widget.text()
                elif isinstance(widget, QCheckBox):
                    value = widget.isChecked()
                else:
                    raise Exception("Unknown widget %s" % widget)

                item[key] = value

            if insert_row_number:
                item["row"] = row

            batch.append(item)

        return batch

    def start_clicked(self):
        if not self.observers_LE.text():
            msg_box = QMessageBox()
            msg_box.setIcon(QMessageBox.Warning)
            msg_box.setText("You must setup observers.")
            msg_box.setWindowTitle("ObserveClient: WARNING")
            msg_box.setStandardButtons(QMessageBox.Ok)
            msg_box.exec()

            return

        self.refresh_statusbar(ClientStatus.RUNNING)
        self.set_enable_widgets(False)

        instrument = self.instrument_CB.currentText().lower()
        current_row = self.observe_TW.currentRow()

        if current_row == -1:
            current_row = 0
            self.observe_TW.setCurrentCell(current_row, 0)

        batch = self.get_batch(current_row, insert_row_number=True)

        self.exit.clear()
        self.stop_event.clear()
        self.thread_exit["instrument"].clear()

        kwargs = {
            "batch": batch,
            "instrument": instrument,
            "observers": self.observers_LE.text(),
            "data_path": self.data_path_CB.currentText(),
            "thread_exit": self.thread_exit["instrument"],
        }

        self.batch_thread = BatchThread(self)

        instrument_worker = Worker(self.batch_thread.run, **kwargs) # Any other args, kwargs are passed to the run function
        instrument_worker.signals.result.connect(self.batch_result)
        instrument_worker.signals.finished.connect(self.batch_finished)
        instrument_worker.signals.progress.connect(self.batch_progress)
        instrument_worker.signals.error.connect(self.batch_error)

        self.threadpool.start(instrument_worker)

    def add_row_clicked(self):
        row = self.observe_TW.currentRow() + 1
        item = self.create_item(enable=True, obj="dark")

        self.observe_TW.insertRow(row)
        self.observe_TW.setCurrentCell(row, 0)

        self.fill_row(row, item)

    def remove_row_clicked(self):
        row = self.observe_TW.currentRow()
        self.observe_TW.removeRow(row)

        #for selection_range in self.observe_TW.selectedRanges():
        #    for idx in range(selection_range.topRow(), selection_range.bottomRow()+1):
        #        print("remove ", idx)
        #        self.observe_TW.removeRow(idx)

    def stop_clicked(self):
        self.stop_event.set()

    def select_target(self):
        target_LE = self.sender()

        self.refresh_statusbar(ClientStatus.RUNNING, "Searching target '%s'..." % target_LE.text())

        self.exit.clear()
        self.thread_exit["target"].clear()

        kwargs = {
            "thread_exit": self.thread_exit["target"],
            "target": target_LE.text(),
        }

        row = self.observe_TW.currentRow()

        # WARNING: Pokud by byla instance objektu SelectTargetThread pouze jako
        # lokalni promena teto metody, tak by se ve vetsine pripadu tato instance
        # uvolnila z pameti drive nez by byla moznost zavolat jeji metody, ktere
        # jsou navazany na jednotlive udalosti.
        self.select_target_thread = SelectTargetThread(self, row)

        target_worker = Worker(self.select_target_thread.run, **kwargs) # Any other args, kwargs are passed to the run function
        target_worker.signals.result.connect(self.select_target_thread.result)
        target_worker.signals.finished.connect(self.select_target_thread.finished)
        target_worker.signals.progress.connect(self.select_target_thread.progress)
        target_worker.signals.error.connect(self.select_target_thread.error)

        self.threadpool.start(target_worker)

    def select_grating_angle(self):
        grating_angle_LE = self.sender()

        dialog = SelectGratingAngleDialog(self.grating_angles, parent=self)

        if not dialog.exec_():
            self.actual_grating_angle = None
            return

        self.actual_grating_angle = dialog.get_selected_grating_angle()

        grating_angle_LE.setGratingAngle(self.actual_grating_angle)

    def export_to_file(self):
        batch = self.get_batch(0)

        save_file_dialog = QFileDialog()
        save_file_dialog.setDirectory(os.path.expanduser("~/observe_client"))
        save_file_dialog.setAcceptMode(QFileDialog.AcceptSave)
        save_file_dialog.setFileMode(QFileDialog.AnyFile)
        save_file_dialog.setFilter(QDir.Files)

        if not save_file_dialog.exec_():
            return

        filename = save_file_dialog.selectedFiles()[0]

        with open(filename, "w") as fo:
            json.dump(batch, fo, sort_keys=False, indent=4)

    def load_from_file(self):
        open_file_dialog = QFileDialog()
        open_file_dialog.setDirectory(os.path.expanduser("~/observe_client"))
        open_file_dialog.setAcceptMode(QFileDialog.AcceptOpen)
        open_file_dialog.setFileMode(QFileDialog.AnyFile)
        open_file_dialog.setFilter(QDir.Files)

        if not open_file_dialog.exec_():
            return

        filename = open_file_dialog.selectedFiles()[0]

        with open(filename, "r") as fo:
            batch = json.load(fo)

        self.load_batch(batch)

    def create_thread_load_from_stars(self, action, idsch=-1):
        self.refresh_statusbar(ClientStatus.RUNNING, "Loading schedule from Stars...")

        self.exit.clear()
        self.thread_exit[action].clear()

        kwargs = {
            "thread_exit": self.thread_exit[action],
        }

        self.load_from_stars_thread = LoadFromStarsThread(self, action, idsch)

        schedule_worker = Worker(self.load_from_stars_thread.run, **kwargs)
        schedule_worker.signals.result.connect(self.load_from_stars_thread.result)
        schedule_worker.signals.finished.connect(self.load_from_stars_thread.finished)
        schedule_worker.signals.progress.connect(self.load_from_stars_thread.progress)
        schedule_worker.signals.error.connect(self.load_from_stars_thread.error)

        self.threadpool.start(schedule_worker)

    def load_from_stars(self):
        self.create_thread_load_from_stars("schedule_list")

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
    observe_client_ui = ObserveClientUI()

    return app.exec()

if __name__ == '__main__':
    main()
