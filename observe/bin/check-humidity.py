#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

#
# $Date$
# $Rev$
# $URL$
#

import os
import sys
import time
import syslog
import subprocess
import signal
import xmlrpclib
import traceback
import MySQLdb
import ConfigParser

def log_exception():
    exc_type, exc_value, exc_traceback = sys.exc_info()
    err_msg = " ".join(traceback.format_exception_only(exc_type, exc_value))
    syslog.syslog(syslog.LOG_ERR, "%s: %s" % (sys.argv[0], err_msg))

class App():
    def __init__(self, cfg_filename):
        self.cfg_filename = cfg_filename
        self.load_cfg()

        signal.signal(signal.SIGALRM, self.handler)

        self.check_humidity()

    def load_cfg(self):
        cfg = ConfigParser.RawConfigParser()
        cfg.read(self.cfg_filename)

        self.max_age_humidity = cfg.getint("main", "max_age_humidity")
        self.max_humidity_outside = cfg.getint("main", "max_humidity_outside")
        self.max_humidity_telescope = cfg.getint("main", "max_humidity_telescope")
        self.file_send_warning = cfg.get("main", "file_send_warning")

        self.mail_to = cfg.get("mail", "to")
        self.mail_subject = cfg.get("mail", "subject")
        self.mail_period = cfg.getint("mail", "period")

        self.sensorid_humidity_outside = cfg.getint("sensorid", "humidity_outside")
        self.sensorid_humidity_telescope = cfg.getint("sensorid", "humidity_telescope")

        self.mysql_server = cfg.get("mysql", "server")
        self.mysql_username = cfg.get("mysql", "username")
        self.mysql_password = cfg.get("mysql", "password")
        self.mysql_db = cfg.get("mysql", "db")
        self.mysql_command = cfg.get("mysql", "command")

        self.telescoped_host = cfg.get("telescoped", "host")
        self.telescoped_port = cfg.getint("telescoped", "port")

    def is_dome_open(self):
        self.result = False
        url = "http://%s:%i" % (self.telescoped_host, self.telescoped_port)

        signal.alarm(3)

        try:
            proxy = xmlrpclib.ServerProxy(url)
            telescope_info = proxy.telescope_info()
            if (telescope_info["glst"].split()[6] != "4"):
                self.result = True
        except Exception:
            self.result = False
            log_exception()

        signal.alarm(0)

        return self.result

    def check_humidity(self):
        connection = MySQLdb.connect(self.mysql_server, self.mysql_username, self.mysql_password, self.mysql_db)
        cursor = connection.cursor(MySQLdb.cursors.DictCursor)
        min_timekey = time.time() - self.max_age_humidity

        cursor.execute(self.mysql_command % (min_timekey))

        hum_out = None
        hum_tel = None
        data = cursor.fetchall()
        for line in data:
            if (line["sensorid"] == self.sensorid_humidity_outside) and (line["timekey"] >= min_timekey):
                hum_out = line["value"]
            elif (line["sensorid"] == self.sensorid_humidity_telescope) and (line["timekey"] >= min_timekey):
                hum_tel = line["value"]

        connection.close()

        if (self.is_dome_open()) and (hum_out > self.max_humidity_outside):
            self.report(hum_out, hum_tel)
        elif (hum_tel > self.max_humidity_telescope):
            self.report(hum_out, hum_tel)

    def report(self, hum_out, hum_tel):
        body = []
        body.append("Vlhkost venku: %s%% (maximalni povolena %s%%)\n" % (hum_out, self.max_humidity_outside))
        body.append("Vlhkost na dalekohledu: %s%% (maximalni povolena %s%%)\n\n" % (hum_tel, self.max_humidity_telescope))
        body.append("Nastaveni limitu: primula:%s\n" % (self.cfg_filename))

        mutt_cmd = 'mutt -s "%s" "%s" <<EOF\n%s\nEOF' % (self.mail_subject, self.mail_to, "".join(body))

        if (os.path.isfile(self.file_send_warning)):
            age = time.time() - os.stat(self.file_send_warning).st_mtime
        else:
            age = self.mail_period

        if (age >= self.mail_period):
            fo = open(self.file_send_warning, "w")
            fo.write(mutt_cmd)
            fo.close()

            subprocess.call(mutt_cmd, shell=True)

    def handler(self, signum, frame):
        self.result = False

def main():
    if (len(sys.argv) != 2):
        raise Exception("cfg_filename not found")
        
    App(sys.argv[1])

if __name__ == '__main__':
    try:
        main()
    except Exception:
        log_exception()
