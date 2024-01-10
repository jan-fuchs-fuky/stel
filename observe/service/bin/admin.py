#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2020 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import os
import sys
import time
import hashlib
import threading
import logging
import invoke
import fabric
import configparser
import traceback

from multiprocessing import Pool
from logging.handlers import TimedRotatingFileHandler
from PyQt5 import uic

from PyQt5.QtCore import QObject, QRunnable, QThreadPool, pyqtSignal, pyqtSlot

from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QSizePolicy, QSpacerItem, \
    QPushButton, QDialog, QLineEdit, QVBoxLayout, QMessageBox

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

ADMIN_UI = "%s/../share/admin.ui" % SCRIPT_PATH
ADMIN_CFG = "%s/../etc/admin.cfg" % SCRIPT_PATH
ADMIN_LOG = "%s/../log/admin_%%s.log" % SCRIPT_PATH

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

def ping(host):
    try:
        result = invoke.run("ping -W 1 -c 3 %s" % host)
    except:
        result = "error"

    return [host, result]

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
        `dict`

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

class ServicesThread():

    def __init__(self, window, services):
        self.window = window
        self.services = services

        print(services)

        self.logger = logging.getLogger("ServicesThread")
        init_logger(self.logger, ADMIN_LOG % "services_thread")
        self.logger.info("Starting process 'ServicesThread'")

    def loop(self, progress_callback):
        data = {}
        connection = {}

        for service in self.services:
            host = self.services[service]["host"]
            if host not in connection:
                self.logger.info("new fabric.Connection(%s)", host)
                connection[host] = fabric.Connection(host)

        while not self.window.exit_event.is_set():

            for service in self.services:
                try:
                    host = self.services[service]["host"]
                    result = connection[host].run("systemctl status %s.service" % service)
                    if isinstance(result, fabric.Result) and result.exited == 0:
                        data[service] = ["running", result.stdout]
                    else:
                        data[service] = ["error", "error"]
                except:
                    traceback.print_exc()
                    data[service] = ["unknown", "unknown"]

            progress_callback.emit(data)
            time.sleep(1)

        return "success"

    def progress_fn(self, data):
        for service in data:
            status, details = data[service]
            self.window.services_widgets[service]["status"].setText(status)
            self.window.services_widgets[service]["status"].setToolTip(details)

    def print_output(self, result):
        self.logger.info("ServicesThread: %s" % result)

    def error_fn(self, error):
        exctype, value, format_exc = error

        self.logger.error("ServicesThread: %s %s\n%s" % (exctype, value, format_exc))

    def thread_complete(self):
        self.logger.info("ServicesThread: completed")

class ComputersThread():

    def __init__(self, window, computers):
        self.window = window
        self.computers = computers

        print(computers)

        self.logger = logging.getLogger("ComputersThread")
        init_logger(self.logger, ADMIN_LOG % "computers_thread")
        self.logger.info("Starting process 'ComputersThread'")

    def loop(self, progress_callback):
        data = {}

        while not self.window.exit_event.is_set():
            pool = Pool(10)
            ping_results = pool.map(ping, self.computers)

            for item in ping_results:
                computer, result = item
                if isinstance(result, invoke.Result) and result.exited == 0:
                    data[computer] = "running"
                else:
                    data[computer] = "off"

            progress_callback.emit(data)
            time.sleep(1)

        return "success"

    def progress_fn(self, data):
        for computer in data:
            self.window.computer_widgets[computer]["status"].setText(data[computer])

    def print_output(self, result):
        self.logger.info("ComputersThread: %s" % result)

    def error_fn(self, error):
        exctype, value, format_exc = error

        self.logger.error("ComputersThread: %s %s\n%s" % (exctype, value, format_exc))

    def thread_complete(self):
        self.logger.info("ComputersThread: completed")

class AdminUI(QMainWindow):

    def __init__(self):
        super(AdminUI, self).__init__()

        self.load_cfg()

        uic.loadUi(ADMIN_UI, self)

        self.logger = logging.getLogger("AdminUI")
        init_logger(self.logger, ADMIN_LOG % "ui")
        self.logger.info("Starting process 'AdminUI'")

        self.services_layout = self.services_groupBox.layout()
        self.computers_layout = self.computers_groupBox.layout()

        self.services_widgets = {}
        row = 1
        for service in self.services:
            self.add_service(self.services[service]["host"], service, row)
            row += 1

        self.services_layout.addItem(self.get_vertical_spacer(), row, 0)

        self.computer_widgets = {}
        row = 1
        for computer in self.computers:
            self.add_computer(computer, row)
            row += 1

        self.computers_layout.addItem(self.get_vertical_spacer(), row, 0)

        self.start_threads()
        self.show()

    def load_cfg(self):
        rcp = configparser.ConfigParser()
        rcp.read(ADMIN_CFG)

        self.services = {}
        self.computers = []

        for section in rcp.sections():
            if (section.startswith("service_")):
                name = section[8:]
                cfg = dict(rcp.items(section))
                self.services[name] = cfg

                if cfg["host"] not in self.computers:
                    self.computers.append(cfg["host"])

        #print(self.services)
        #print(self.computers)

    def get_vertical_spacer(self):
        vertical_spacer = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)

        return vertical_spacer

    def add_computer(self, name, row):
        widgets = {
            "name": QLabel(name),
            "status": QLabel("unknown"),
            "shutdown": QPushButton("shutdown"),
            "reboot": QPushButton("reboot"),
            "wol": QPushButton("wol"),
        }

        self.computer_widgets[name] = {}
        column = 0
        for item in widgets.keys():
            self.computer_widgets[name][item] = widgets[item]
            self.computers_layout.addWidget(widgets[item], row, column)
            column += 1

        for item in ["shutdown", "reboot", "wol"]:
            # IMPORTANT: You must pass the additional data in as a named
            # parameter on the lambda to create a new namespace. Otherwise
            # the value of item will be bound to the final value in the parent
            # for loop (always "wol").
            widgets[item].clicked.connect(lambda state, host=name, action=item: self.computer_action(host, action))

    def computer_action(self, host, action):
        print("computer", host, action)

    def add_service(self, host, name, row):
        widgets = {
            "name": QLabel(name),
            "computer": QLabel(host),
            "status": QLabel("unknown"),
            "start": QPushButton("start"),
            "stop": QPushButton("stop"),
            "restart": QPushButton("restart"),
        }

        self.services_widgets[name] = {}
        column = 0
        for item in widgets.keys():
            self.services_widgets[name][item] = widgets[item]
            self.services_layout.addWidget(widgets[item], row, column)
            column += 1

        for action in ["start", "stop", "restart"]:
            widgets[action].clicked.connect(lambda state, host=host, service=name, action=action: self.service_action(host, service, action))

    def service_action(self, host, service, action):
        print("service", host, service, action)

    def start_threads(self):
        self.threadpool = QThreadPool()
        self.threadpool.setMaxThreadCount(10)

        self.exit_event = threading.Event()

        self.services_thread = ServicesThread(self, self.services)
        self.computers_thread = ComputersThread(self, self.computers)

        self.exit_event.clear()

        for thread in [self.services_thread, self.computers_thread]:
            worker = Worker(thread.loop) # Any other args, kwargs are passed to the run function
            worker.signals.result.connect(thread.print_output)
            worker.signals.finished.connect(thread.thread_complete)
            worker.signals.progress.connect(thread.progress_fn)
            worker.signals.error.connect(thread.error_fn)

            self.threadpool.start(worker)

    def stop_threads(self):
        self.threadpool.clear()

        self.exit_event.set()

        self.threadpool.waitForDone()

    def closeEvent(self, event):
        self.stop_threads()

        event.accept()
        #event.ignore()

class Login(QDialog):

    def __init__(self):
        super(Login, self).__init__()

        self.username_LE = QLineEdit(self)

        self.password_LE = QLineEdit(self)
        self.password_LE.setEchoMode(QLineEdit.Password)

        self.login_BT = QPushButton("Login", self)
        self.login_BT.clicked.connect(self.login_action)

        self.exit_BT = QPushButton("Exit", self)
        self.exit_BT.clicked.connect(self.exit_action)

        layout = QVBoxLayout(self)
        layout.addWidget(self.username_LE)
        layout.addWidget(self.password_LE)
        layout.addWidget(self.login_BT)
        layout.addWidget(self.exit_BT)

    def login_action(self):
        password_hash = hashlib.sha256(self.password_LE.text().encode("ascii")).hexdigest()

        if self.username_LE.text() == "stel" and password_hash == "19ab4a444432f4d20d31b977ac670beb7ce242bedf6a17b8ba5e1079596bec97":
            self.accept()
        else:
            QMessageBox.warning(self, "Error", "Bad username or password")

    def exit_action(self):
        self.reject()

def main():
    app = QApplication([])
    login = Login()

    #if login.exec_() == QDialog.Accepted:
    if 1:
        admin_ui = AdminUI()
        app.exec()

if __name__ == '__main__':
    main()
