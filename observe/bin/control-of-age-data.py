#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import time
import requests
import traceback
import MySQLdb
import MySQLdb.cursors
import configparser

class App():
    def __init__(self, cfg_filename):
        self.cfg_filename = cfg_filename
        self.load_cfg()

        msg = ""
        for service in self.services:
            result = self.run_control(service)
            if (result != 1):
                msg += "%s:%s:%s\n" % (service["host"][0], service["daemon"], result)

        reported_filename = self.reported_filename % ("observe")
        if (msg != ""):
            reported = True
            if (os.path.isfile(reported_filename)):
                reported_fo = open(reported_filename, "r")
                reported_time = float(reported_fo.readline().strip())
                reported_fo.close()

                if (reported_time > (time.time() - self.repeat_time)):
                    reported = False

            if (reported):
                outdated_subject = self.outdated_subject
                outdated_body = self.outdated_body % (msg)
                send_mail(self.mail_to, outdated_subject, outdated_body)
                send_telegram(outdated_subject, msg)

                reported_fo = open(reported_filename, "w")
                reported_fo.write("%s\n" % time.time())
                reported_fo.close()

        elif (os.path.isfile(reported_filename)):
            os.remove(reported_filename)
            current_subject = self.current_subject
            current_body = self.current_body
            send_mail(self.mail_to, current_subject, current_body)
            send_telegram(current_subject, self.current_body.split("\n")[0])

    def run_control(self, service):
        mysql_command = self.mysql_command % (service["host"], service["port"], service["daemon"])

        date = self.get_date(mysql_command)
        offset = time.time() - time.mktime(date.timetuple())

        if (offset > self.offset_max):
            return 0

        return 1

    def seconds2human(self, value):
        if (value >= 3600):
            h = value / 3600
            value %= 3600
            m = value / 60
            s = value % 60
            return "%02i:%02i:%02i" % (h, m, s)
        elif (value >= 60):
            m = value / 60
            s = value % 60
            return "00:%02i:%02i" % (m, s)

        return "00:00:%02i" % value

    def load_cfg(self):
        cfg = configparser.ConfigParser()
        cfg.read(self.cfg_filename)

        self.offset_max = cfg.getint("main", "offset_max")
        self.repeat_time = cfg.getint("main", "repeat_time")
        self.mail_to = cfg.get("main", "mail_to")
        self.sms_to = cfg.get("main", "sms_to")
        self.reported_filename = cfg.get("main", "reported_filename")

        self.outdated_subject = cfg.get("outdated", "subject")
        self.outdated_body = cfg.get("outdated", "body")

        self.current_subject = cfg.get("current", "subject")
        self.current_body = cfg.get("current", "body")

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

    def get_date(self, command):
        connection = MySQLdb.connect(self.mysql_server, self.mysql_username, self.mysql_password, self.mysql_db)
        cursor = connection.cursor(MySQLdb.cursors.DictCursor)

        cursor.execute(command)
        data = cursor.fetchall()
        connection.close()

        return data[0]["date"]

def send_mail(to, subject, body):
    os.system('mutt -s "%s" "%s" <<EOF\n%s\nEOF' % (subject, to, body))

def send_telegram(subject, message):

    params = {
        "chat_id": "-1001896652443",
        "text": "%s\n\n%s" % (subject, message),
    }

    response = requests.get("https://api.telegram.org/bot6186356935:AAGMLDOT5y_ffBxzUBykwOpVLAnNNx4_kl8/sendMessage", params=params)

    if response.status_code != 200:
        subject = "%s Telegram failed" % os.path.basename(sys.argv[0])
        err_msg = "Telegram status_code = %i" % response.status_code
        send_mail("fuky@asu.cas.cz", subject, err_msg)

def main():
    if (len(sys.argv) != 2):
        raise Exception("cfg_filename not found")

    App(sys.argv[1])

if __name__ == '__main__':
    try:
        main()
    except Exception:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        err_msg = " ".join(traceback.format_exception(exc_type, exc_value, exc_traceback))
        #print(err_msg)
        send_mail("fuky@asu.cas.cz", "%s failed" % (sys.argv[0]), err_msg)
