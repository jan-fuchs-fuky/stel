#! /usr/bin/env python2.7
# -*- coding: utf-8 -*-

#
#  Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
#
#  Copyright (C) 2010-2017 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

#
# a color name (e.g. "red", "orange", "navajo white" as defined in the X Window
# file rgb.txt) - /etc/X11/rgb.txt
#

import pygtk
pygtk.require("2.0")
import gtk
import gobject
import sys
import gobject
import logging
import getopt
import ssl
import urllib
import urllib2
import traceback
import ConfigParser
import lxml.etree as etree
from StringIO import StringIO
from threading import Thread, Event

__author__ = 'Jan Fuchs'
__version__ = 'v2017.01.16'

def user_permissions_version():
    print """\
\
user_permissions %s
Written by Jan Fuchs <fuky@asu.cas.cz>

Copyright (C) 2010-2017 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. \
\
    """ % (__version__)

def user_permissions_help():
    print """\
\
Usage:

    %s -c /opt/user_permissions/etc/user_permissions.cfg

Arguments:

    -c, --config FILE    path to configuration file 
    -h, --help           display this help and exit
    -v, --version        output version information and exit \
\
    """ % sys.argv[0]

class UserPermissions(object):
    def __init__(self, cfg, logger):
        gtk.gdk.threads_init()

        self.exit = False
        self.cfg = cfg
        self.logger = logger
        self.save_results = {}
        self.permissions = {}
        self.pulse_return = False

        logger.info("Starting user_permissions %s" % (__version__))

        self.color_red = gtk.gdk.color_parse("#FFCCCC")
        self.color_green = gtk.gdk.color_parse("#CCFFCC")

        self.urllib_init()
        self.create_gui()

        self.client_event = Event()

        self.client_TH = Thread(target = self.client_thread)
        self.client_TH.start()

        gtk.main()

    def urllib_init(self):
        self.realm = self.cfg.get("observe", "realm")
        self.uri = self.cfg.get("observe", "uri")
        self.user = self.cfg.get("observe", "user")
        self.password = self.cfg.get("observe", "password")
        self.url = self.cfg.get("observe", "url")
        
        auth_handler = urllib2.HTTPBasicAuthHandler()
        auth_handler.add_password(self.realm, self.uri, self.user, self.password)

        context = ssl._create_unverified_context()
        ssl_unverified_context_handler = urllib2.HTTPSHandler(context=context)

        opener = urllib2.build_opener(ssl_unverified_context_handler, auth_handler)

        urllib2.install_opener(opener)

    def create_gui(self):
        window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        window.connect("delete_event", self.quit)

        self.vbox = gtk.VBox(False, 5)
        window.add(self.vbox)
        self.vbox.show()

        self.statusBar = gtk.Statusbar()
        self.context_id = self.statusBar.get_context_id("global")
        self.vbox.pack_end(self.statusBar, True, True)
        self.statusBar.show()

        self.createTreeView()

        self.progressBar = gtk.ProgressBar()
        self.vbox.pack_start(self.progressBar, True, True)
        self.progressBar.show()

        self.saveButton = gtk.Button("Save")
        self.saveButton.connect("clicked", self.saveButtonClicked, None)
        self.vbox.pack_end(self.saveButton, True, True)
        self.saveButton.show()

        window.show()

    def getUsers(self):
        request = urllib2.Request(self.url)
        request.add_header("Accept", "text/xml")

        try:
            result = urllib2.urlopen(request)
            msg = "%s %s" % (result.code, result.msg)
            self.statusBar.push(self.context_id, msg)
            if (result.code != 200):
                return
        except Exception:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            err_msg = " ".join(traceback.format_exception_only(exc_type, exc_value)).strip()
            self.statusBar.push(self.context_id, err_msg)
            return

        xml = result.read()

        users = []
        users_etree = etree.parse(StringIO(xml))
        user_elts = users_etree.xpath("/users/user")
        for user_elt in user_elts:
            user = {}
            for children in user_elt.getchildren():
                user.update({children.tag: children.text})
            users.append(user)

        self.permissions.clear()
        self.listStore.clear()
        for user in users:
            if (self.save_results.has_key(user["login"])):
                save_result = self.save_results[user["login"]]

                if (save_result.startswith("200")):
                    color = "LightGreen"
                else:
                    color = "LightPink"
            else:
                save_result = ""
                color = "White"

            self.listStore.append([
                user["login"],
                user["firstName"],
                user["lastName"],
                user["email"],
                user["permission"],
                save_result,
                color,
            ])

            self.permissions.update({user["login"]: user["permission"]})

    def setUsers(self):
        self.save_results.clear()
        item = self.listStore.get_iter_first()
        while (item != None):
            login = self.listStore.get_value(item, 0)
            permission = self.listStore.get_value(item, 4)
            item = self.listStore.iter_next(item)

            if (self.permissions[login] == permission):
                continue

            params = {}
            params.update({"login": login})
            params.update({"permission": permission})
            params.update({"password": ""})

            try:
                result = urllib2.urlopen(self.url, urllib.urlencode(params))
                self.save_results.update({login: "%s %s" % (result.code, result.msg)})
            except Exception:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                save_result = " ".join(traceback.format_exception_only(exc_type, exc_value)).strip()
                self.save_results.update({login: save_result})

    def createTreeView(self):
        listStorePermissions = gtk.ListStore(str)
        permissions = ["none", "read", "control"]
        for item in permissions:
            listStorePermissions.append([item])

        self.listStore = gtk.ListStore(str, str, str, str, str, str, str)
        treeView = gtk.TreeView(self.listStore)
        
        cellRendererText = gtk.CellRendererText()

        cellRendererCombo = gtk.CellRendererCombo()
        cellRendererCombo.set_property("editable", True)
        cellRendererCombo.set_property("model", listStorePermissions)
        cellRendererCombo.set_property("text-column", 0)
        cellRendererCombo.connect("edited", self.permissionChanged, self.listStore)

        columns = [
            ["Login", cellRendererText],
            ["First Name", cellRendererText],
            ["Last Name", cellRendererText],
            ["E-mail", cellRendererText],
            ["Permission", cellRendererCombo],
            ["Save result", cellRendererText],
        ]

        id = 0
        for item in columns:
            column = gtk.TreeViewColumn(item[0], item[1], text=id, background=6)
            treeView.append_column(column)
            id += 1
        
        self.getUsers()

        self.vbox.pack_start(treeView, True, True)
        treeView.show()

    def permissionChanged(self, widget, path, text, model):
        login = model[path][0]

        if (text in ["none", "read", "control"]):
            model[path][4] = text

            if (self.permissions[login] != text):
                model[path][6] = "LightBlue"
            else:
                model[path][6] = "White"

    def saveButtonClicked(self, widget, event):
        self.saveButton.set_sensitive(False)
        self.client_event.set()

    def pulse(self):
        self.progressBar.pulse()

        if (self.pulse_return == False):
            self.progressBar.set_fraction(0.0)

        return self.pulse_return

    def client_thread(self):
        while (not self.exit):
            self.client_event.wait()
            if (self.exit):
                break

            self.pulse_return = True
            gobject.timeout_add(100, self.pulse)

            self.setUsers()
            gobject.idle_add(self.getUsers)

            self.pulse_return = False
            gobject.idle_add(self.progressBar.set_fraction, 0.0)

            self.client_event.clear()
            self.saveButton.set_sensitive(True)

    def quit(self, widget, event):
        self.exit = True
        self.client_event.set()
        gtk.main_quit()

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
                user_permissions_help()
                sys.exit()
            elif o in ("-v", "--version"):
                user_permissions_version()
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
        logger = logging.getLogger("user_permissions")
        logger.setLevel(logging.DEBUG)

        formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] %(threadName)s[%(thread)d] - %(levelname)s - %(message)s")

        fh = logging.FileHandler("%s/user_permissions.log" % (
            cfg.get("user_permissions", "log_path")))
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(formatter)

        logger.addHandler(fh)
    except Exception, err:
        show_error_dialog("Create logger failed\n%s." % (repr(err)))
        sys.exit(1)

    try:
        user_permissions = UserPermissions(cfg, logger)
    except Exception, err:
        logger.exception("UserPermissions() failed")

if __name__ == "__main__":
    main()
