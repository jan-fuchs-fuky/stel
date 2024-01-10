#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2019 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
# https://www.learnpyqt.com/courses/concurrent-execution/multithreading-pyqt-applications-qthreadpool/
#


import os
import sys
import re
import time
import xmlrpc.client
import configparser
import traceback

from datetime import datetime
from PyQt5 import QtWidgets, uic
from PyQt5.QtCore import QRunnable, QThreadPool, QObject, pyqtSlot, pyqtSignal

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

SERVICE_UI = "%s/../share/service.ui" % SCRIPT_PATH

SERVICE_CFG = "%s/../etc/service.cfg" % SCRIPT_PATH
SOURCE_COORDINATES_CFG = "%s/../etc/source_coordinates.cfg" % SCRIPT_PATH
STAR_COORDINATES_CFG = "%s/../etc/star_coordinates.cfg" % SCRIPT_PATH

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

class ServiceUI(QtWidgets.QMainWindow):

    def __init__(self):
        super(ServiceUI, self).__init__()

        self.load_cfg()

        uic.loadUi(SERVICE_UI, self)

        self.load_star_coordinates()

        self.star_ra_value_LB.setText("HELLO")
        self.star_ra_value_LB.setStyleSheet("background-color: #CCFFCC;");

        self.star_dec_value_LB.setText("WORLD")

        self.source_coordinatec_save_BT.clicked.connect(self.source_coordinatec_save)
        self.source_coordinates_go_BT.clicked.connect(self.source_coordinates_go)
        self.star_coordinates_save_BT.clicked.connect(self.star_coordinates_save)
        self.star_coordinates_go_BT.clicked.connect(self.star_coordinates_go)
        self.do0_on_BT.clicked.connect(self.do0_on)
        self.do0_off_BT.clicked.connect(self.do0_off)
        self.do1_on_BT.clicked.connect(self.do1_on)
        self.do2_on_BT.clicked.connect(self.do2_on)
        self.do3_on_BT.clicked.connect(self.do3_on)
        self.do4_on_BT.clicked.connect(self.do4_on)
        self.do1_off_BT.clicked.connect(self.do1_off)
        self.do2_off_BT.clicked.connect(self.do2_off)
        self.do3_off_BT.clicked.connect(self.do3_off)
        self.do4_off_BT.clicked.connect(self.do4_off)
        self.do5_on_BT.clicked.connect(self.do5_on)
        self.do5_off_BT.clicked.connect(self.do5_off)
        self.dome_decrement_BT.clicked.connect(self.dome_decrement)
        self.dome_increment_BT.clicked.connect(self.dome_increment)
        self.dec_decrement_BT.clicked.connect(self.dec_decrement)
        self.ra_decrement_BT.clicked.connect(self.ra_decrement)
        self.dec_increment_BT.clicked.connect(self.dec_increment)
        self.ra_increment_BT.clicked.connect(self.ra_increment)
        self.stop_star_BT.clicked.connect(self.stop_star)
        self.stop_dome_BT.clicked.connect(self.stop_dome)
        self.stop_move_BT.clicked.connect(self.stop_move)
        self.up_BT.clicked.connect(self.up)
        self.down_BT.clicked.connect(self.down)
        self.left_BT.clicked.connect(self.left)
        self.right_BT.clicked.connect(self.right)

        self.star_coordinates_load_CM.currentTextChanged.connect(self.star_coordinates_load)

        self.star_coordinates_load_CM.addItems(self.star_coordinates.keys())

        self.log_TE.setReadOnly(True)

        self.threadpool = QThreadPool()
        self.threadpool.setMaxThreadCount(2)

        print("Multithreading with maximum %d threads" % self.threadpool.maxThreadCount())

        main_worker = Worker(self.execute_main) # Any other args, kwargs are passed to the run function
        main_worker.signals.result.connect(self.print_output)
        main_worker.signals.finished.connect(self.thread_complete)
        main_worker.signals.progress.connect(self.progress_fn)
        main_worker.signals.error.connect(self.error_fn)

        self.threadpool.start(main_worker)

        worker = Worker(self.execute_this_fn) # Any other args, kwargs are passed to the run function
        worker.signals.result.connect(self.print_output)
        worker.signals.finished.connect(self.thread_complete)
        worker.signals.progress.connect(self.progress_fn)

        #self.threadpool.start(worker)
        #self.threadpool.start(worker)
        #self.threadpool.start(worker)
        #self.threadpool.start(worker)

        self.show()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(SERVICE_CFG)

        self.telescope = dict(rcp.items("telescope"))

    def telescope_main_loop(self, progress_callback, proxy):
        values = proxy.telescope_execute("TRRD")

        # 095006.487 -390212.10 0
        ra, dec, position = values.split()

        telescope_state = {
            "ra": ra,
            "dec": dec,
            "position": int(position),
        }

        values = proxy.telescope_execute("TRHD")

        # -0.0024 -38.9402
        ha, da = values.split()
        telescope_state["ha"] = ha
        telescope_state["da"] = da

        digital_outputs = [ 0, 0, 0, 0, 0, 0 ]
        for idx in range(6):
            digital_outputs[idx] = proxy.telescope_execute("GLRO %i" % idx)

        telescope_state["digital_outputs"] = digital_outputs

        values = proxy.telescope_execute("TRGV")
        pointing_ra, pointing_dec = values.split()
        telescope_state["pointing_ra"] = pointing_ra
        telescope_state["pointing_dec"] = pointing_dec

        progress_callback.emit(telescope_state)

    def execute_main(self, progress_callback):
        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.telescope)

        while (True):
            self.telescope_main_loop(progress_callback, proxy)
            time.sleep(1)

        return "Done."

    def execute_this_fn(self, progress_callback):
        for n in range(0, 5):
            time.sleep(1)
            progress_callback.emit(n*100/4)

        return "Done."

    def progress_fn(self, telescope_state):
        #self.centralwidget.setEnabled(False)
        #self.log_TE.textCursor().insertHtml("<b>%s: </b>%d%% done<br>" % (datetime.now(), n))

        self.pointing_ra_value_LB.setText(telescope_state["pointing_ra"])
        self.pointing_dec_value_LB.setText(telescope_state["pointing_dec"])
        self.pointing_ra_value_LB.setStyleSheet("background-color: #CCFFCC;");
        self.pointing_dec_value_LB.setStyleSheet("background-color: #CCFFCC;");

        self.star_ra_value_LB.setText(telescope_state["ra"])
        self.star_dec_value_LB.setText(telescope_state["dec"])
        self.star_ra_value_LB.setStyleSheet("background-color: #CCFFCC;");
        self.star_dec_value_LB.setStyleSheet("background-color: #CCFFCC;");

        position_str = "east"
        if (telescope_state["position"] == 1):
            position_str = "western"

        self.star_coordinates_position_value_LB.setText(position_str)
        self.star_coordinates_position_value_LB.setStyleSheet("background-color: #CCFFCC;");

        self.source_coordinates_ha_value_LB.setText(telescope_state["ha"])
        self.source_coordinates_da_value_LB.setText(telescope_state["da"])
        self.source_coordinates_ha_value_LB.setStyleSheet("background-color: #CCFFCC;");
        self.source_coordinates_da_value_LB.setStyleSheet("background-color: #CCFFCC;");

        digital_outputs_labels = [
            self.do0_value_LB,
            self.do1_value_LB,
            self.do2_value_LB,
            self.do3_value_LB,
            self.do4_value_LB,
            self.do5_value_LB,
        ]

        for idx in range(6):
            digital_outputs_labels[idx].setText(telescope_state["digital_outputs"][idx])
            digital_outputs_labels[idx].setStyleSheet("background-color: #CCFFCC;");

        self.statusbar.showMessage("Connected %(host)s:%(port)s" % self.telescope)
        self.statusbar.setStyleSheet("background-color: #CCFFCC;");

    def print_output(self, s):
        print(s)

    def error_fn(self, error):
        dt_str = datetime.now().strftime("%H:%M:%S")

        exctype, value, format_exc = error

        self.statusbar.showMessage("%s %s" % (exctype, value))
        self.statusbar.setStyleSheet("background-color: #FFCCCC;");

        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#FF0000">%s %s</font><br>' % (exctype, value))

    def show_msg(self, msg):
        dt_str = datetime.now().strftime("%H:%M:%S")

        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('%s<br>' % msg)

    def show_error(self, msg):
        dt_str = datetime.now().strftime("%H:%M:%S")

        self.log_TE.textCursor().insertHtml("<b>%s: </b>" % dt_str)
        self.log_TE.textCursor().insertHtml('<font color="#FF0000">%s</font><br>' % msg)

    def thread_complete(self):
        #self.centralwidget.setEnabled(True)
        print("THREAD COMPLETE!")

    def load_star_coordinates(self):
        with open(STAR_COORDINATES_CFG, "r") as fo:
            lines = fo.readlines()

        self.star_coordinates = { "empty": ["", ""] }

        for line in lines:
            name, value = line.strip().split("=")
            name = name.strip()
            ra, dec = value.strip().split()

            self.star_coordinates[name] = [ra, dec]

    def star_coordinates_load(self, name):
        ra, dec = self.star_coordinates[name]

        self.star_coordinates_name_LE.setText(name)
        self.star_coordinates_ra_LE.setText(ra)
        self.star_coordinates_dec_LE.setText(dec)

    def star_coordinates_go(self):
        ra = self.star_coordinates_ra_LE.text()
        dec = self.star_coordinates_dec_LE.text()
        position = self.star_coordinates_position_CB.currentIndex()

        ra_pattern = re.compile("^[0-9]{6}\.[0-9]{2}$")
        dec_pattern = re.compile("^[+-]{0,1}[0-9]{6}\.[0-9]{2}$")

        result = ra_pattern.search(ra)
        if (not result):
            self.show_error("Unexpected RA value '%s' (FORMAT: HHMMSS.SS)" % ra)
            return

        result = dec_pattern.search(dec)
        if (not result):
            self.show_error("Unexpected DEC value '%s' (FORMAT: -HHMMSS.SS)" % dec)
            return

        params = {}
        params["ra"] =  ra
        params["dec"] = dec
        params["position"] = position
        params["object"] = self.star_coordinates_name_LE.text()

        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.telescope)

        result = proxy.telescope_set_coordinates(params).strip()
        self.show_msg("telescope_set_coordinates(%s) => %s" % (params, result))

        cmd = "TGRA 1"
        result = proxy.telescope_execute(cmd).strip()
        self.show_msg("%s => %s" % (cmd, result))

    def stop_star(self):
        self.telescope_execute("TGRA 0")

    def star_coordinates_save(self):
        name = self.star_coordinates_name_LE.text()
        ra = self.star_coordinates_ra_LE.text()
        dec = self.star_coordinates_dec_LE.text()

        self.star_coordinates[name] = [ra, dec]

        self.star_coordinates_load_CM.addItem(name)

        with open(STAR_COORDINATES_CFG, "w") as fo:
            for name in self.star_coordinates:
                if (name != "empty"):
                    ra, dec = self.star_coordinates[name]
                    fo.write("%s = %s %s\n" % (name, ra, dec))

    def telescope_execute(self, cmd):
        proxy = xmlrpc.client.ServerProxy("http://%(host)s:%(port)s" % self.telescope)

        result = proxy.telescope_execute(cmd).strip()
        self.show_msg("%s => %s" % (cmd, result))

    def source_coordinatec_save(self):
        pass
    def source_coordinates_go(self):
        pass

    def do0_on(self):
        self.telescope_execute("GLSO 0 1")

    def do0_off(self):
        self.telescope_execute("GLSO 0 0")

    def do1_on(self):
        self.telescope_execute("GLSO 1 1")

    def do2_on(self):
        self.telescope_execute("GLSO 2 1")

    def do3_on(self):
        self.telescope_execute("GLSO 3 1")

    def do4_on(self):
        self.telescope_execute("GLSO 4 1")

    def do1_off(self):
        self.telescope_execute("GLSO 1 0")

    def do2_off(self):
        self.telescope_execute("GLSO 2 0")

    def do3_off(self):
        self.telescope_execute("GLSO 3 0")

    def do4_off(self):
        self.telescope_execute("GLSO 4 0")

    def do5_on(self):
        self.telescope_execute("GLSO 5 1")

    def do5_off(self):
        self.telescope_execute("GLSO 5 0")

    def dome_decrement(self):
        value = self.dome_DSB.value() * -1.0
        self.telescope_execute("DOSR %.2f" % value)
        self.telescope_execute("DOGR")

    def dome_increment(self):
        value = self.dome_DSB.value()
        self.telescope_execute("DOSR %.2f" % value)
        self.telescope_execute("DOGR")

    def stop_dome(self):
        self.telescope_execute("DOST")

    def dec_decrement(self):
        value = self.telescope_move_DSB.value() * -1.0
        self.telescope_execute("TSRR 0.00 %.2f" % value)
        self.telescope_execute("TGRR 1")

    def dec_increment(self):
        value = self.telescope_move_DSB.value()
        self.telescope_execute("TSRR 0.00 %.2f" % value)
        self.telescope_execute("TGRR 1")

    def ra_decrement(self):
        value = self.telescope_move_DSB.value() * -1.0
        self.telescope_execute("TSRR %.2f 0.00" % value)
        self.telescope_execute("TGRR 1")

    def ra_increment(self):
        value = self.telescope_move_DSB.value()
        self.telescope_execute("TSRR %.2f 0.00" % value)
        self.telescope_execute("TGRR 1")

    def stop_move(self):
        self.telescope_execute("TGRR 0")

    def up(self):
        value = self.pointing_move_DSB.value() * -1.0
        self.telescope_execute("TSGC 0.0 %.1f" % value)

    def down(self):
        value = self.pointing_move_DSB.value()
        self.telescope_execute("TSGC 0.0 %.1f" % value)

    def left(self):
        value = self.pointing_move_DSB.value()
        self.telescope_execute("TSGC %.1f 0.0" % value)

    def right(self):
        value = self.pointing_move_DSB.value() * -1.0
        self.telescope_execute("TSGC %.1f 0.0" % value)

def main():
    app = QtWidgets.QApplication([])
    service_ui = ServiceUI()

    return app.exec()

if __name__ == '__main__':
    main()
