#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

#
#  Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
#  $Date$
#  $Rev$
#  $URL$
#
#  Copyright (C) 2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#
#  This file is part of Observe (Observing System for Ondrejov).
#
#  Observe is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Observe is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Observe.  If not, see <http://www.gnu.org/licenses/>.
#

import pygtk
pygtk.require("2.0")
import gtk
import sys
import time
import xmlrpclib
import gobject
import logging
import getopt
import ConfigParser
from threading import Thread

__author__ = 'Jan Fuchs'
__date__ = '$Date$'
__version__ = '$Rev$'

def telescope_oil_version():
    print """\
\
telesope-oil %s (%s)
Written by Jan Fuchs <fuky@sunstel.asu.cas.cz>

Copyright (C) 2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. \
\
    """ % (__version__, __date__)

def telesope_oil_help():
    print """\
\
Usage:

    %s -c /opt/observe/etc/telesope-oil.cfg

Arguments:

    -c, --config FILE    path to configuration file 
    -h, --help           display this help and exit
    -v, --version        output version information and exit \
\
    """ % sys.argv[0]

class TelescopeOil(object):

    def __init__(self, cfg, logger):
        gtk.gdk.threads_init()

        self.oil_on = False
        self.on_start_time = None
        self.on_stop_time = None
        self.cfg = cfg
        self.logger = logger

        logger.info("Starting telescope-oil %s (%s)" % (__version__, __date__))

        self.color_red = gtk.gdk.color_parse("#FFCCCC")
        self.color_green = gtk.gdk.color_parse("#CCFFCC")

        self.builder = gtk.Builder()
        self.builder.add_from_file(cfg.get("main", "gui_xml"))

        signals2methods = {
            "oil_on_BT_clicked" : self.oil_on_BT_clicked,
            "oil_off_BT_clicked" : self.oil_off_BT_clicked,
            "on_window_delete" : self.delete,
        }

        self.builder.connect_signals(signals2methods)

        self.window = self.builder.get_object("window")
        self.window.set_title("Telescope OIL")

        self.oil_EB = self.builder.get_object("oil_EB")
        self.oil_LB = self.builder.get_object("oil_LB")

        self.timeout_on_SB = self.builder.get_object("timeout_on_SB")
        self.timeout_on_SB.set_value(cfg.getint("main", "timeout_on"))

        self.statusbar = self.builder.get_object("statusbar")
        self.context_id = self.statusbar.get_context_id("global")
        self.telescope_rpc = xmlrpclib.ServerProxy("http://%s:%i" % (
            cfg.get("telescoped", "host"),
            cfg.getint("telescoped", "port")))
        self.quit = False

        self.log_buffer = self.builder.get_object("log_TV").get_buffer()
        
        self.tag = self.log_buffer.create_tag('info')
        self.tag.set_property('foreground', '#0000FF')
        
        self.tag = self.log_buffer.create_tag('error')
        self.tag.set_property('foreground', '#FF0000')

        self.tag = self.log_buffer.create_tag('command')
        self.tag.set_property('foreground', '#000000')

        self.client_TH = Thread(target = self.client_thread)
        self.client_TH.start()

        self.window.show()
        gtk.main()

    def client_thread(self):
        logger = logging.getLogger("telescope-oil.rpc_client")

        try:
            self.client_thread_loop()
        except Exception, err:
            logger.exception("TelescopeOil.client_thread_loop() failed")
            #gobject.idle_add(self.statusbar.push, self.context_id, repr(err))

        logger.info("rpc_client thread exiting...")

    def client_thread_loop(self):
        oil_state = {
            '0': "OFF",
            '1': "START1",
            '2': "START2",
            '3': "START3",
            '4': "ON",
            '5': "OFF_DELAY",
        }

        on_time = 0
        off_time = 0
        while (not self.quit):
            time.sleep(0.25)

            info = self.telescope_rpc.telescope_info()

            # 0 OFF - olej je vypnuty
            # 1 START1 - kontrola tlaku dusiku
            # 2 START2 - start cerpadel
            # 3 START3 - stabilizace tlak
            # 4 ON - zapnuty olej
            # 5 OFF_DELAY - prodleva pred zastavenim oleje
            glst = info["glst"].split()

            if (glst[0] == '4'):
                oil_color = self.color_green
            else:
                oil_color = self.color_red

            gobject.idle_add(self.oil_EB.modify_bg, gtk.STATE_NORMAL, oil_color)
            gobject.idle_add(self.oil_LB.set_text, oil_state[glst[0]])

            if ((self.oil_on) and (glst[0] == 4)):
                continue
            elif ((not self.oil_on) and (glst[0] == 0)):
                continue

            if ((self.oil_on) and (time.time() >= self.on_stop_time)):
                self.oil_on = False
                gobject.idle_add(self.add_log, "error", "Timeout ON")

            cmd = ""
            if ((self.oil_on) and (glst[0] == '0')):
                if ((time.time() - on_time) > 1.0):
                    cmd = "OION 1"
                    on_time = time.time()
            elif ((not self.oil_on) and (glst[0] not in ['0', '5'])):
                if ((time.time() - off_time) > 1.0):
                    cmd = "OION 0"
                    off_time = time.time()

            if (cmd):
                gobject.idle_add(self.add_log, "info", 'Oil = %s' % oil_state[glst[0]])
                gobject.idle_add(self.add_log, "command", 'telescope_execute("%s")' % cmd)
                result = self.telescope_execute_th(cmd)

                if (result == '1'):
                    level = "info"
                else:
                    level = "error"

                gobject.idle_add(self.add_log, level, 'Answer: %s' % result)
                last_cmd = time.time()

    def push_statusbar_th(self, status):
        pass
        #gobject.idle_add(self.statusbar.push, self.context_id, status)

    def telescope_execute_th(self, cmd):
        result = ""

        try:
            result = self.telescope_rpc.telescope_execute(cmd)
            self.push_statusbar_th("Command: %s, Answer: %s" % (cmd, result))
        except Exception, err:
            self.logger.exception("TelescopeOil.telescope_execute() failed")
            self.push_statusbar_th(repr(err))
            result = repr(err)

        return result

    def add_log(self, level, message):
        self.logger.info("%s: %s" % (level, message))

        actual_time = time.gmtime()

        h = actual_time[3]
        m = actual_time[4]
        s = actual_time[5]

        message = "[%02i:%02i:%02i] %s" % (h, m, s, message)

        start = self.log_buffer.get_iter_at_offset(0)
        self.log_buffer.insert(start, message + "\n")
        stop = self.log_buffer.get_iter_at_offset(0)
        self.log_buffer.apply_tag_by_name(level, start, stop)

    def oil_on_BT_clicked(self, widget):
        self.on_start_time = time.time()
        self.on_stop_time = time.time() + (self.timeout_on_SB.get_value() * 60)
        self.add_log("info", "Oil ON clicked, Timeout ON = %0.0f minutes" % (self.timeout_on_SB.get_value()))

        self.oil_on = True

    def oil_off_BT_clicked(self, widget):
        self.add_log("info", "Oil OFF clicked")

        self.oil_on = False

    def delete(self, window, event):
        self.logger.info("exiting...")
        self.quit = True
        gtk.main_quit()
        return True

def show_error_dialog(errmsg):
    md = gtk.MessageDialog(
        None, 
        gtk.DIALOG_DESTROY_WITH_PARENT,
        gtk.MESSAGE_ERROR, 
        gtk.BUTTONS_CLOSE,
        errmsg)

    md.run()
    md.destroy()

def main():
    config = None

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hvc:", [ "help", "version", "config=" ])

        for o, a in opts:
            if o in ("-h", "--help"):
                telesope_oil_help()
                sys.exit()
            elif o in ("-v", "--version"):
                telescope_oil_version()
                sys.exit()
            elif o in ("-c", "--config"):
                config = a

        if (config is None):
            raise Exception("Option --config is required")
    except Exception, err:
        show_error_dialog("Parse command line arguments failed.\n\n%s" % (repr(err)))
        sys.exit(1)

    try:
        cfg = ConfigParser.RawConfigParser()
        cfg.read(config)
    except Exception, err:
        show_error_dialog("Load configuration failed\n\n%s." % (repr(err)))
        sys.exit(1)

    try:
        logger = logging.getLogger("telescope-oil")
        logger.setLevel(logging.DEBUG)

        formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] %(threadName)s[%(thread)d] - %(levelname)s - %(message)s")

        fh = logging.FileHandler("%s/telescope-oil.log" % (cfg.get("main", "log_path")))
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(formatter)

        logger.addHandler(fh)
    except Exception, err:
        show_error_dialog("Create logger failed\n%s." % (repr(err)))
        sys.exit(1)

    try:
        telescopeOil = TelescopeOil(cfg, logger)
    except Exception, err:
        logger.exception("TelescopeOil() failed")

if __name__ == "__main__":
    main()
