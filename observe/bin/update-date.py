#! /usr/bin/env python
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#

import os
import sys
import time
import syslog
import signal
import xmlrpclib
import urllib
import urllib2
import traceback
import MySQLdb
import ConfigParser
import ssl
ssl._create_default_https_context = ssl._create_unverified_context

def log_exception():
    exc_type, exc_value, exc_traceback = sys.exc_info()
    err_msg = " ".join(traceback.format_exception_only(exc_type, exc_value))
    syslog.syslog(syslog.LOG_ERR, "%s: %s" % (sys.argv[0], err_msg))

class App():
    def __init__(self, cfg_filename):
        self.cfg_filename = cfg_filename
        self.load_cfg()

        signal.signal(signal.SIGALRM, self.handler)

        for service in self.services:
            if (self.check_service(service)):
                self.update_date(service)

    def load_cfg(self):
        cfg = ConfigParser.RawConfigParser()
        cfg.read(self.cfg_filename)

        self.realm = cfg.get("observe", "realm")
        self.uri = cfg.get("observe", "uri")
        self.user = cfg.get("observe", "user")
        self.password = cfg.get("observe", "password")
        self.url = cfg.get("observe", "url")

        self.mysql_server = cfg.get("mysql", "server")
        self.mysql_username = cfg.get("mysql", "username")
        self.mysql_password = cfg.get("mysql", "password")
        self.mysql_db = cfg.get("mysql", "db")
        self.mysql_command = cfg.get("mysql", "command")

        self.services = []
        for section in cfg.sections():
            if (section.startswith("service-")):
                options = {}
                options.update({"host": cfg.get(section, "host")})
                options.update({"port": cfg.getint(section, "port")})
                options.update({"daemon": cfg.get(section, "daemon")})
                self.services.append(options)

    def check_service(self, service):
        self.result = True
        url = "http://%s:%i" % (service["host"], service["port"])

        signal.alarm(3)

        try:
            if (service["daemon"] == "spectrographd"):
                proxy = xmlrpclib.ServerProxy(url)
                if (not proxy.spectrograph_execute("GLST")[0].isdigit()):
                    self.result = False
            elif (service["daemon"] == "telescoped"):
                proxy = xmlrpclib.ServerProxy(url)
                if (not proxy.telescope_execute("GLST")[0].isdigit()):
                    self.result = False
            elif (service["daemon"] == "observed"):
                self.check_service_observed()
        except Exception:
            self.result = False
            log_exception()

        signal.alarm(0)

        return self.result

    def check_service_observed(self):
        auth_handler = urllib2.HTTPBasicAuthHandler()
        auth_handler.add_password(self.realm, self.uri, self.user, self.password)
        opener = urllib2.build_opener(auth_handler)
        urllib2.install_opener(opener)

        request = urllib2.Request(self.url)
        request.add_header("Accept", "text/xml")

        result = urllib2.urlopen(request)
        msg = "%s %s" % (result.code, result.msg)
        if (result.code != 200):
            raise Exception(msg)

    def update_date(self, service):
        connection = MySQLdb.connect(self.mysql_server, self.mysql_username, self.mysql_password, self.mysql_db)
        cursor = connection.cursor(MySQLdb.cursors.DictCursor)

        cursor.execute("UPDATE daemons SET date=NOW() WHERE host like '%s' AND port = %i AND daemon like '%s'" %
            (service["host"], service["port"], service["daemon"]))
        connection.close()

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
