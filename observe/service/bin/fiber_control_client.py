#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019-2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# https://www.learnpyqt.com/courses/concurrent-execution/multithreading-pyqt-applications-qthreadpool/
#
# $ ssh -L 6001:192.168.193.198:6001 -L 6000:192.168.193.198:6000 -L 8888:192.168.193.195:8888 -L 9999:192.168.193.195:9999 sulafat
#

import os
import sys
import time
import xmlrpc.client
import configparser
import traceback
import threading
import xmlrpc.client

from datetime import datetime
from PyQt5 import uic
from PyQt5.QtWidgets import QMessageBox, QMainWindow, QPushButton, QApplication
from PyQt5.QtCore import QRunnable, QThreadPool, QObject, pyqtSlot, pyqtSignal

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

FIBER_CONTROL_UI = "%s/../share/fiber_control.ui" % SCRIPT_PATH

FIBER_CONTROL_CFG = "%s/../etc/fiber_control_client.cfg" % SCRIPT_PATH

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
    progress = pyqtSignal(dict, list, float, str)

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
        self.thread_exit.clear()

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

class FiberControlUI(QMainWindow):

    def __init__(self):
        super(FiberControlUI, self).__init__()

        self.load_cfg()

        self.exit = threading.Event()

        self.thread_exit = {}
        for key in ["main", "telescope"]:
            self.thread_exit[key] = threading.Event()
            self.thread_exit[key].set()

        uic.loadUi(FIBER_CONTROL_UI, self)

        self.iodine_cell_position2str = [
            "STOP",
            "inserted",
            "removed",
            "movement",
            "ALARM",
        ]

        self.value_labels = {
            "esw1a": self.focus_end_switch_a_value_LB,
            "esw1b": self.focus_end_switch_b_value_LB,
            "esw2a": self.cameras_end_switch_b_value_LB,
            "esw2b": self.cameras_end_switch_a_value_LB,
            "g1": self.g1_power_value_LB,
            "g2": self.g2_power_value_LB,
            "focus_position": self.focus_position_value_LB,
            "cameras_position": self.cameras_position_value_LB,
            "voltage_33": self.voltage_33_value_LB,
            "voltage_50": self.voltage_50_value_LB,
            "voltage_120": self.voltage_120_value_LB,
        }

        self.telescope_parking_log_LB.setText("")

        self.focus_position_CB.currentTextChanged.connect(self.focus_position_load)
        self.camera_position_CB.currentTextChanged.connect(self.camera_position_load)

        self.focus_position_CB.addItems(self.focus.keys())
        self.camera_position_CB.addItems(self.camera.keys())

        self.focus_position_go_BT.clicked.connect(self.focus_position_go)
        self.focus_position_increment_BT.clicked.connect(self.focus_position_increment)
        self.focus_position_decrement_BT.clicked.connect(self.focus_position_decrement)
        self.cameras_position_go_BT.clicked.connect(self.cameras_position_go)
        self.cameras_position_increment_BT.clicked.connect(self.cameras_position_increment)
        self.cameras_position_decrement_BT.clicked.connect(self.cameras_position_decrement)
        self.reset_BT.clicked.connect(self.reset)
        self.iodine_cell_insert_BT.clicked.connect(self.iodine_cell_insert)
        self.iodine_cell_remove_BT.clicked.connect(self.iodine_cell_remove)
        self.goto_telescope_parking_BT.clicked.connect(self.goto_telescope_parking)

        for idx in range(1, 5):
            for action in ["on", "off"]:
                button = getattr(self, "relay_%i_%s_BT" % (idx, action))
                button.clicked.connect(self.set_quido_relay_clicked)

        self.cameras_all_off_RB.toggled.connect(lambda:self.cameras_power(self.cameras_all_off_RB, 0))
        self.cameras_g1_on_RB.toggled.connect(lambda:self.cameras_power(self.cameras_g1_on_RB, 1))
        self.cameras_g2_on_RB.toggled.connect(lambda:self.cameras_power(self.cameras_g2_on_RB, 2))
        self.cameras_all_on_RB.toggled.connect(lambda:self.cameras_power(self.cameras_all_on_RB, 3))

        self.threadpool = QThreadPool()
        self.threadpool.setMaxThreadCount(2)

        print("Multithreading with maximum %d threads" % self.threadpool.maxThreadCount())

        kwargs = {
            "thread_exit": self.thread_exit["main"],
        }

        main_worker = Worker(self.execute_main, **kwargs) # Any other args, kwargs are passed to the run function
        main_worker.signals.result.connect(self.print_output)
        main_worker.signals.finished.connect(self.thread_complete)
        main_worker.signals.progress.connect(self.progress_fn)
        main_worker.signals.error.connect(self.error_fn)

        self.threadpool.start(main_worker)

        #worker = Worker(self.execute_this_fn) # Any other args, kwargs are passed to the run function
        #worker.signals.result.connect(self.print_output)
        #worker.signals.finished.connect(self.thread_complete)
        #worker.signals.progress.connect(self.progress_fn)
        #self.threadpool.start(worker)

        self.proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.toptec)
        self.quido_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.quido)
        self.spectrograph_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.spectrograph)
        self.telescope_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.telescope)

        self.show()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(FIBER_CONTROL_CFG)

        self.toptec = dict(rcp.items("toptec"))
        self.focus = dict(rcp.items("focus"))
        self.camera = dict(rcp.items("camera"))
        self.quido = dict(rcp.items("quido"))
        self.spectrograph = dict(rcp.items("spectrograph"))
        self.telescope = dict(rcp.items("telescope"))

        for key in self.focus:
            self.focus[key] = int(self.focus[key])

        for key in self.camera:
            self.camera[key] = int(self.camera[key])

    def set_quido_relay_clicked(self):
        button = self.sender()
        if not isinstance(button, QPushButton):
            return

        # relay_4_on_BT => ['relay', '4', 'on', 'BT']
        prefix, relay_number, action, suffix = button.objectName().split("_")

        relay_number = int(relay_number)

        value = 0
        if action == "on":
            value = 1

        # pc-tubus
        if relay_number == 4:
            answer = QMessageBox.question(self, "WARNING",
                "WARNING: Do you really want to shut down INDUSTRIAL COMPUTER on 2M tubus?", QMessageBox.Yes | QMessageBox.No)
            if answer == QMessageBox.No:
                return

        self.quido_proxy.quido_set_relay(relay_number, value)

    def focus_position_load(self, name):
        self.focus_position_SB.setValue(self.focus[name])

    def camera_position_load(self, name):
        self.cameras_position_SB.setValue(self.camera[name])

    def cameras_power(self, radio_button, value):
        if radio_button.isChecked():
            #print(radio_button.text(), value)
            self.proxy.toptec_set_camera_power(value)

    def focus_position_go(self):
        value = self.focus_position_SB.value()
        self.proxy.toptec_set_focus_position(value)

    def focus_position_increment(self):
        self.proxy.toptec_inc_focus_position()

    def focus_position_decrement(self):
        self.proxy.toptec_dec_focus_position()

    def focus_position_save(self):
        pass

    def cameras_position_save(self):
        pass

    def cameras_position_go(self):
        value = self.cameras_position_SB.value()
        self.proxy.toptec_set_camera_position(value)

    def cameras_position_increment(self):
        self.proxy.toptec_inc_camera_position()

    def cameras_position_decrement(self):
        self.proxy.toptec_dec_camera_position()

    def reset(self):
        self.proxy.toptec_reset()

    def iodine_cell_insert(self):
        print("iodine_cell_insert")
        self.spectrograph_proxy.spectrograph_execute("SPCH 26 2")

    def iodine_cell_remove(self):
        print("iodine_cell_remove")
        self.spectrograph_proxy.spectrograph_execute("SPCH 26 1")

    def telescope_progress_fn(self, values, dummy1, dummy2, dummy3):

        color = "#CCFFCC"
        msg = values["info"]

        if values["error"]:
            color = "#FFCCCC"
            msg = values["error"]

        self.telescope_parking_log_LB.setText(msg)
        self.telescope_parking_log_LB.setStyleSheet("background-color: %s;" % color)

    def wait_on_hour_dec_position(self, proxy):
        telescope_info = proxy.telescope_info()

        items = telescope_info["glst"].split()

        if ((items[1] == "3") and (items[2] == "1") and (items[3] == "1")):
            return True

        return False

    def wait_on_dome_slit_close(self, proxy):
        telescope_info = proxy.telescope_info()

        items = telescope_info["glst"].split()

        if items[6] == "4":
            return True

        return False

    def telescope_execute_main(self, progress_callback):
        telescope_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.telescope)

        cmds = []
        cmds.append("DOST")                             # DOme STop
        cmds.append("TETR 0")                           # TElescope TRack
        cmds.append("DOSO 0")                           # DOme Slit Open (0 - CLOSE)
        cmds.append("FTOC 0")                           # Flap Tube Open or Close (0 - CLOSE)
        cmds.append("FMOC 0")                             # Flap Mirror Open or Close (0 - CLOSE)
        cmds.append("DOSA %s" % self.telescope["dosa"]) # DOme Set Absolute position
        cmds.append("DOGA")                             # DOme Go Absolute position
        cmds.append("TSHA 10 80")
        cmds.append("TGHA 1")
        cmds.append("TEHC 1")
        cmds.append("TEDC 1")
        # Telescope Set new Hour and declination axis Absolute
        cmds.append("TSHA %s %s" % (self.telescope["tsha_hour"], self.telescope["tsha_dec"]))
        cmds.append("TGHA 1")
        cmds.append("TEON 0")                           # TElescope ON or off
        cmds.append("OION 0")                           # OIl ON or OFF (0 - OFF)

        try:
            for cmd in cmds:
                if self.exit.is_set():
                    return

                result = telescope_proxy.telescope_execute(cmd).strip()
                if (result == "") or (result != "1"):
                    raise Exception("Telescope not accept '%s'" % cmd)
                else:
                    msg = {
                        "info": "Telescope accept '%s'" % cmd,
                        "error": "",
                    }
                    progress_callback.emit(msg, [], 0.0, "")
                    time.sleep(1)

                if (cmd in ["TGHA 1", "TEHC 1", "TEDC 1", "DOSO 0"]):
                    time.sleep(3)
                    while not self.exit.is_set():
                        if cmd != "DOSO 0" and self.wait_on_hour_dec_position(telescope_proxy):
                            break
                        elif cmd == "DOSO 0" and self.wait_on_dome_slit_close(telescope_proxy):
                            break
                        else:
                            time.sleep(1)
                    else:
                        return

            spectrograph_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.spectrograph)
            spectrograph_proxy.spectrograph_execute("SPCH 26 1") # Iodine cell remove
        except Exception:
            msg = {
                "info": "",
                "error": traceback.format_exc(),
            }
            progress_callback.emit(msg, [], 0.0, "")
            return

        msg = {
            "info": "Dome and telescope are parked.",
            "error": "",
        }
        progress_callback.emit(msg, [], 0.0, "")

    def goto_telescope_parking(self):
        print("goto_telescope_parking")

        answer = QMessageBox.question(self, "WARNING",
            "WARNING: Do you really want run telescope parking?", QMessageBox.Yes | QMessageBox.No)
        if answer == QMessageBox.No:
            return

        kwargs = {
            "thread_exit": self.thread_exit["telescope"],
        }

        telescope_worker = Worker(self.telescope_execute_main, **kwargs) # Any other args, kwargs are passed to the run function
        telescope_worker.signals.result.connect(self.print_output)
        telescope_worker.signals.finished.connect(self.thread_complete)
        telescope_worker.signals.progress.connect(self.telescope_progress_fn)
        telescope_worker.signals.error.connect(self.error_fn)

        self.threadpool.start(telescope_worker)

    def toptec_main_loop(self, proxy, quido_proxy, spectrograph_proxy, progress_callback):
        values = proxy.toptec_get_values()
        #print(values)

        relays = quido_proxy.quido_get_relays()
        #print(relays)

        temperature = quido_proxy.quido_get_temperature()
        #print(temperature)

        iodine_cell_position = spectrograph_proxy.spectrograph_execute("SPGS 26")
        #print("iodine_cell_position = '%s'" % iodine_cell_position)

        try:
            iodine_cell_position = self.iodine_cell_position2str[int(iodine_cell_position)]
        except:
            iodine_cell_position = "UNKNOWN"

        progress_callback.emit(values, relays, temperature, iodine_cell_position)

    def execute_main(self, progress_callback):
        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.toptec)
        quido_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.quido)
        spectrograph_proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.spectrograph)

        while not self.exit.is_set():
            self.toptec_main_loop(proxy, quido_proxy, spectrograph_proxy, progress_callback)
            time.sleep(1)

        return "Done."

    def execute_this_fn(self, progress_callback):
        for n in range(0, 5):
            time.sleep(1)
            progress_callback.emit(n*100/4)

        return "Done."

    def progress_fn(self, values, relays, temperature, iodine_cell_position):
        #self.centralwidget.setEnabled(False)
        #self.log_TE.textCursor().insertHtml("<b>%s: </b>%d%% done<br>" % (datetime.now(), n))

        values_str = {}
        colors = {}

        for key in values:
            colors[key] = "#CCFFCC"

            if key.startswith("esw") or (key in ["g1", "g2"]):
                values_str[key] = "OFF"
                if (values[key]):
                    values_str[key] = "ON"

                if (key in ["g1", "g2"]):
                    if (not values[key]):
                        colors[key] = "#FFCCCC"
                else:
                    if (values[key]):
                        colors[key] = "#FFCCCC"
            elif key.endswith("_position"):
                values_str[key] = "%i" % values[key]
            elif key.startswith("voltage_"):
                values_str[key] = "%.1fV" % (values[key] / 10.0)

        self.focus_end_switch_a_value_LB.setText(values_str["esw1a"])
        self.focus_end_switch_b_value_LB.setText(values_str["esw1b"])

        self.cameras_end_switch_b_value_LB.setText(values_str["esw2a"])
        self.cameras_end_switch_a_value_LB.setText(values_str["esw2b"])

        self.g1_power_value_LB.setText(values_str["g1"])
        self.g2_power_value_LB.setText(values_str["g2"])

        self.focus_position_value_LB.setText(values_str["focus_position"])
        self.cameras_position_value_LB.setText(values_str["cameras_position"])

        self.voltage_33_value_LB.setText(values_str["voltage_33"])
        self.voltage_50_value_LB.setText(values_str["voltage_50"])
        self.voltage_120_value_LB.setText(values_str["voltage_120"])

        temperature_color = "#CCFFCC"
        temperature_str = "%.1f°C" % temperature
        if temperature < 0 or temperature > 40:
            temperature_color = "#FFCCCC"

        self.switchboard_temperature_value_LB.setText(temperature_str)
        self.switchboard_temperature_value_LB.setStyleSheet("background-color: %s;" % temperature_color)

        for key in self.value_labels:
            self.value_labels[key].setText(values_str[key])
            self.value_labels[key].setStyleSheet("background-color: %s;" % colors[key])

        iodine_cell_position_color = "#CCFFCC"
        if iodine_cell_position != "inserted":
            iodine_cell_position_color = "#FFCCCC"

        self.iodine_cell_position_value_LB.setText(iodine_cell_position)
        self.iodine_cell_position_value_LB.setStyleSheet("background-color: %s;" % iodine_cell_position_color)

        self.statusbar.showMessage("Connected %(host)s:%(port)s" % self.toptec)
        self.statusbar.setStyleSheet("background-color: #CCFFCC;")

        # TODO: dodelat poradne
        for idx in range(len(relays)):
            label = getattr(self, "relay%i_LB" % (idx+1))

            value = "OFF"
            color = "#FFCCCC"
            if relays[idx] == 1:
                value = "ON"
                color = "#CCFFCC"

            label.setText(value)
            label.setStyleSheet("background-color: %s;" % color)

    def print_output(self, s):
        print(s)

    def error_fn(self, error):
        dt_str = datetime.now().strftime("%H:%M:%S")

        exctype, value, format_exc = error

        self.statusbar.showMessage("%s %s" % (exctype, value))
        self.statusbar.setStyleSheet("background-color: #FFCCCC;")

        #self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        #self.log_TE.textCursor().insertHtml('<font color="#FF0000">error</font><br>')

    def thread_complete(self):
        #self.centralwidget.setEnabled(True)
        print("THREAD COMPLETE!")

    def closeEvent(self, event):
        print("closeEvent(event = %s)" % event)
        self.exit.set()

        all_thread_exit = False

        while not all_thread_exit:
            all_thread_exit = True

            for key in self.thread_exit:
                if self.thread_exit[key].is_set():
                    print("Thread '%s' is exited." % key)
                else:
                    all_thread_exit = False

            if not all_thread_exit:
                print("Waiting...")
                time.sleep(1)

        print("GUI exiting...")

def main():
    app = QApplication([])
    service_ui = FiberControlUI()

    return app.exec()

if __name__ == '__main__':
    main()
