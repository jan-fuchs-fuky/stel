#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
# $Date$
# $Rev$
#

import pygtk
pygtk.require("2.0")
import gtk
import gtk.gdk as gdk
import gtk.glade as glade
import lxml.etree as etree
import gobject
import os
import sys
import re
import getopt
import string
import time
import select
import socket
import subprocess
import gettext
from threading import Thread, Condition

__version__ = '2.0.0'

patterns = {}
patterns.update({"INSTRUMENT": re.compile("INSTRUMENT = ([^ \n]*)")})
#patterns.update({"PATH": re.compile("PATH = ([^ \n]*)")})
#patterns.update({"ARCHIVEPATH": re.compile("ARCHIVEPATH = ([^ \n]*)")})
patterns.update({"PATHS": re.compile("PATHS = ([^ \n]*)")})
patterns.update({"ARCHIVEPATHS": re.compile("ARCHIVEPATHS = ([^ \n]*)")})
patterns.update({"reading_out_ccd": re.compile("reading out CCD ([0-9]*) ([0-9]*) '(.*)'")})
patterns.update({"exposing": re.compile("exposing ([0-9]*) ([0-9]*) '(.*)'")})
patterns.update({"ccd_is_ready": re.compile("ccd is ready '(.*)'")})

glade.bindtextdomain("peso", "../lang")
glade.textdomain("peso")
_ = gettext.gettext

def peso_help():
    print("Usage: %s [OPTIONS]" % sys.argv[0])
    print("    -c, --config-xml PATH    path to configuration file")
    print("    -g, --glade-xml PATH     path to glade interface")
    print("        --help               display this help and exit")
    print("        --version            output version information and exit")

def peso_version():
    print("PESO %s" % __version__)
    print("Written by Jan Fuchs <fuky@sunstel.asu.cas.cz>\n")
    print("Copyright (C) 2004-2009 Free Software Foundation, Inc.")
    print("This is free software; see the source for copying conditions.  There is NO")
    print("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.")

def values2menu(values):
    menu = gtk.Menu()

    for item in values:
        item_MI = gtk.MenuItem(item)
        menu.add(item_MI)
        item_MI.show()

    return menu

class Peso:
    output_paths = []
    archive_paths = []
    SA_cmds = []
    objects = []
    widgets = {}
    paths = {}
    client_TH = None
    disconnect = False
    condition = None

    action_abort = False
    action_readout = False
    action_quit = False
    target_exposure = 0
    target_MI = None
    target_M = None
    list_target = None
    previous_object = None
    previous_exposure = 0
    question_exposure = False

    def __init__(self):
        self.paths.update({"glade_xml": "../share/peso.glade"})
        self.paths.update({"config_xml": "../share/peso.xml"})

        options = "c:g:hv"
        long_options = ["glade-xml=", "peso-xml=", "help", "version"]
        try:
            opts, args = getopt.getopt(sys.argv[1:], options, long_options)
        except getopt.GetoptError:
            peso_help()
            sys.exit(1)

        for o, a in opts:
            if o in ("-c", "--config-xml"):
                self.paths["config_xml"] = a
            elif o in ("-g", "--glade-xml"):
                self.paths["glade_xml"] = a
            elif o in ("-v", "--version"):
                peso_version()
                sys.exit()
            elif o in ("-h", "--help"):
                peso_help()
                sys.exit()

        self.condition = Condition()

        self.xml = glade.XML(self.paths["glade_xml"], "window")
        self.xml.signal_autoconnect(self)
        
        for widget in self.xml.get_widget_prefix(""):
            self.widgets.update({widget.get_name(): widget})

        self.load_config()

        self.widgets["window"].set_title("PESO %s" % __version__)
        self.widgets["main_NB"].set_current_page(3)

        menu = gtk.Menu()
        for item in self.objects:
            item_MI = gtk.MenuItem(item)
            menu.add(item_MI)
            item_MI.show()
        self.widgets["object_OM"].set_menu(menu)

        self.target_MI = item_MI
        self.target_M = gtk.Menu()
        item_MI = gtk.MenuItem(_("Other")) 
        item_MI.connect("activate", self.fill_target, '') 
        self.target_M.add(item_MI)
        self.target_MI.set_submenu(self.target_M)
        item_MI.show()

        self.widgets["instrument_EB"].modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("black"))
        self.widgets["instrument_title_LB"].modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
        self.widgets["instrument_title_LB"].set_markup("<b>Instrument:</b>")
        self.widgets["instrument_LB"].modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))

        self.widgets["state_EB"].modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("black"))
        self.widgets["state_title_LB"].modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
        self.widgets["state_title_LB"].set_markup("<b>State:</b>")
        self.widgets["state_LB"].modify_fg(gtk.STATE_NORMAL, gtk.gdk.color_parse("white"))
        self.widgets["state_LB"].set_markup("<b>unknown</b>")

        #self.widgets["output_path_OM"].set_menu(values2menu(self.output_paths))
        #self.widgets["output_path_OM"].set_history(0)
        #self.widgets["output_path_ET"].set_text(self.output_paths[0])

        #self.widgets["archive_path_OM"].set_menu(values2menu(self.archive_paths))
        #self.widgets["archive_path_OM"].set_history(0)
        #self.widgets["archive_path_ET"].set_text(self.archive_paths[0])
        
        self.widgets["readout_BT"].set_sensitive(False)
        self.widgets["abort_BT"].set_sensitive(False)     
        self.widgets["disconnect_BT"].set_sensitive(False)     
        
        self.widgets["progressbar_LB"].set_text('')

        self.widgets["count_PB"].set_child_visible(False)
        self.log_buffer = self.widgets["log_TV"].get_buffer()
        
        self.tag = self.log_buffer.create_tag('info')
        self.tag.set_property('foreground', '#0000FF')
        
        self.tag = self.log_buffer.create_tag('error')
        self.tag.set_property('foreground', '#FF0000')

        self.tag = self.log_buffer.create_tag('command')
        self.tag.set_property('foreground', '#000000')

        self.object_OM_changed()
        self.archive_CB_toggled()
        self.autogenerate_filename_CB_toggled()
        self.sensitive_disconnects()

        gtk.main()

    def xmlget_list(self, path):
        items = []
        elements = self.config_tree.xpath(path)
        for e in elements:
            items.append(e.text)
        return items
    
    def xmlget_str(self, path):
        try:
            return self.config_tree.xpath(path)[0].text
        except:
            return None
    
    def xmlget_bool(self, path):
        try:
            if (int(self.xmlget_str(path))):
                return True
        except:
            pass
        return False
    
    def xmlget_int(self, path):
        try:
            return int(self.xmlget_str(path))
        except:
            return 0

    def load_config(self):
        try:
            self.config_tree = etree.parse(self.paths["config_xml"])
        except:
            print("Could not load configuration")
            return

        self.objects = self.xmlget_list("/peso/menus/object/item")
        #self.output_paths = self.xmlget_list("/peso/menus/output_path/item")
        #self.output_paths.append("other")
        #self.archive_paths = self.xmlget_list("/peso/menus/archive_path/item")
        #self.archive_paths.append("other")

        self.widgets["instrument_LB"].set_markup("<b>%s</b>" % self.xmlget_str("/peso/instrument"))
        self.widgets["ip_ET"].set_text(self.xmlget_str("/peso/ip"))
        self.widgets["port_ET"].set_text(self.xmlget_str("/peso/port"))
        self.widgets["username_ET"].set_text(self.xmlget_str("/peso/username"))
        self.widgets["password_ET"].set_text(self.xmlget_str("/peso/password"))
        self.widgets["archive_CB"].set_active(self.xmlget_bool("/peso/archive"))
        #self.widgets["autogenerate_filename_CB"].set_active(self.xmlget_bool("/peso/autogenerate_filename"))
        self.widgets["autogenerate_filename_CB"].set_active(True)
        self.widgets["autogenerate_filename_CB"].set_sensitive(False)
        self.widgets["ccd_temp_SB"].set_value(self.xmlget_int("/peso/ccd/temp"))

    def window_question(self, question):
        xml_question_D = glade.XML(self.paths["glade_xml"], 'question_D')
        xml_question_D.signal_autoconnect(self)
        question_D  = xml_question_D.get_widget("question_D")
        question_LB = xml_question_D.get_widget("question_LB")
        question_LB.set_text(question)

        result = question_D.run()
        question_D.destroy()
        
        if (result == gtk.RESPONSE_YES):
            return True
        else:
            return False

    def object_OM_changed(self, widget=None):
        self.widgets["exposure_SB"].set_range(1, 100000)
        self.widgets["exposure_SB"].set_sensitive(True)

        if (self.previous_object != self.widgets["object_OM"].get_history()):
            self.question_exposure = True
        else:
            self.question_exposure = False

        self.previous_object = self.widgets["object_OM"].get_history()
        self.previous_exposure = self.widgets["exposure_SB"].get_value()

        # target
        if (self.widgets["object_OM"].get_history() == 4):
            self.widgets["objectn_LB"].set_sensitive(True)
            self.widgets["object_ET"].set_sensitive(True)
            self.widgets["exposure_SB"].set_value(self.target_exposure)
            self.widgets["exposure_OM"].set_sensitive(True)
            self.widgets["exposure_SB"].set_sensitive(True)
        else:
            self.widgets["exposure_OM"].set_sensitive(False)
            self.widgets["exposure_OM"].set_history(0)
            self.widgets["objectn_LB"].set_sensitive(False)
            self.widgets["object_ET"].set_sensitive(False)
            self.widgets["object_ET"].set_text(self.objects[self.widgets["object_OM"].get_history()])
            
            # zero
            if (self.widgets["object_OM"].get_history() == 2):
                self.widgets["exposure_SB"].set_range(0, 0)
                self.widgets["exposure_SB"].set_sensitive(False)

    def archive_CB_toggled(self, widget=None):  
         if self.widgets["archive_CB"].get_active():
             self.widgets["archive_path_ET"].set_sensitive(True)
             self.widgets["archive_path_LB"].set_sensitive(True)
             self.widgets["archive_path_OM"].set_sensitive(True)
         else:
             self.widgets["archive_path_ET"].set_sensitive(False)          
             self.widgets["archive_path_LB"].set_sensitive(False)          
             self.widgets["archive_path_OM"].set_sensitive(False)
            
    def autogenerate_filename_CB_toggled(self, widget=None):
        active = self.widgets["autogenerate_filename_CB"].get_active()
        self.widgets["file_ET"].set_sensitive(not active)

        #if (active):
        #    self.condition.acquire()
        #    self.SA_cmds.append("GETKEY FILENAME\n")
        #    self.condition.release()

    def exposure_SB_default_range(self):
        # not zero
        if (self.widgets["object_OM"].get_history() != 2):
            self.widgets["exposure_SB"].set_range(1, 100000)

    def connection_sensitive(self, value):
        names = [
            "connect_BT",
            "ip_ET", 
            "port_ET", 
            "username_ET", 
            "password_ET", 
        ]

        for name in names:
            self.widgets[name].set_sensitive(value)

    def sensitive_connects(self):
        self.connection_sensitive(False)
        self.sensitive_true_on_ready()
        self.widgets["command_ET"].set_sensitive(True)
        self.widgets["execute_BT"].set_sensitive(True)
        self.widgets["readout_BT"].set_sensitive(False)
        self.widgets["abort_BT"].set_sensitive(False)

    def sensitive_disconnects(self):
        self.connection_sensitive(True)
        self.sensitive_false_on_exposing()
        self.sensitive_false_on_ready()
        self.widgets["command_ET"].set_sensitive(False)
        self.widgets["execute_BT"].set_sensitive(False)
        self.widgets["disconnect_BT"].set_sensitive(False)

    def sensitive_false_on_exposing(self):
        names = [
            "count_SB", 
            "exposure_OM", 
            "observer_ET", 
            "object_OM", 
            "object_ET", 
            "speed_OM", 
            "file_ET", 
            "expose_BT", 
            "output_path_ET", 
            "archive_path_ET", 
            "archive_CB", 
            "output_path_OM", 
            "archive_path_OM", 
            "autogenerate_filename_CB", 
        ]

        for name in names:
            self.widgets[name].set_sensitive(False)

    def sensitive_true_on_exposing(self):
        names = [
            "readout_BT", 
            "abort_BT", 
        ]

        for name in names:
            self.widgets[name].set_sensitive(True)

        # not zero
        if (self.widgets["object_OM"].get_history() != 2):
            self.widgets["add_time_BT"].set_sensitive(True)

    def sensitive_on_exposing(self):
        self.sensitive_false_on_exposing()
        self.sensitive_true_on_exposing()

    def sensitive_true_on_ready(self):
        names = [
            "count_SB", 
            "exposure_OM", 
            "observer_ET", 
            "object_OM", 
            "object_ET", 
            "speed_OM", 
            "expose_BT", 
            "output_path_ET", 
            "archive_path_ET", 
            "archive_CB", 
            "output_path_OM", 
        ]

        for name in names:
            self.widgets[name].set_sensitive(True)

        self.archive_CB_toggled()
        self.object_OM_changed()
        self.autogenerate_filename_CB_toggled()

    def sensitive_false_on_ready(self):
        names = [
            "readout_BT", 
            "abort_BT", 
            "add_time_BT", 
        ]

        for name in names:
            self.widgets[name].set_sensitive(False)

    def sensitive_on_ready(self):
        self.sensitive_false_on_ready()
        self.sensitive_true_on_ready()

    # Ctrl+w
    def delete_log_buffer(self, widget):
        self.log_buffer.set_text('')

    def fill_target(self, widget, string):
        self.widgets["object_ET"].set_text(string)
            
    def expose_BT_clicked(self, widget):
        self.widgets["expose_BT"].set_sensitive(False)
        self.action_abort = False
        self.action_readout = False

        self.widgets["exposure_SB"].set_range(self.widgets["exposure_SB"].get_value(), 100000)

        if (self.question_exposure and (self.previous_exposure == self.widgets["exposure_SB"].get_value())):
            self.question_exposure = False
            if (not self.window_question("Exposure %is?" % self.widgets["exposure_SB"].get_value())):
                return
     
        if ((not self.widgets["autogenerate_filename_CB"].get_active()) and (self.widgets["file_ET"].get_text() == "")):
            self.log('error', _("Set name out FITs file"))
            return

        if ((self.widgets["object_OM"].get_history() == 4) and (self.widgets["object_ET"].get_text() == "")):
            self.log('error', _("Set name object"))
            return
        
        if (self.widgets["observer_ET"].get_text() == ""):
            self.log('error', _("Set names observers"))
            return
        
        if (self.widgets["object_OM"].get_history() == 4):
            self.target_exposure = self.widgets["exposure_SB"].get_value()
            if ((self.list_target == None) or (string.find(self.list_target, self.widgets["object_ET"].get_text()) == -1)):
                if (self.list_target == None):
                    self.list_target = self.widgets["object_ET"].get_text() + ';'
                else:
                    self.list_target = self.list_target + self.widgets["object_ET"].get_text() + ';'
                     
                item_MI = gtk.MenuItem(self.widgets["object_ET"].get_text()) 
                item_MI.connect("activate", self.fill_target, self.widgets["object_ET"].get_text()) 
                self.target_M.add(item_MI)
                self.target_MI.set_submenu(self.target_M)
                item_MI.show()

        time = self.widgets["exposure_SB"].get_value()
        count = self.widgets["count_SB"].get_value()

        imagetype = self.objects[self.widgets["object_OM"].get_history()]
        archive = 0
        if (self.widgets["archive_CB"].get_active()):
            archive = 1

        self.condition.acquire()
        self.SA_cmds.append("SET PATH %s\n" % self.widgets["output_path_ET"].get_text())
        if (archive):
            self.SA_cmds.append("SET ARCHIVEPATH %s\n" % self.widgets["archive_path_ET"].get_text())
        self.SA_cmds.append("SET ARCHIVE %i\n" % archive)
        self.SA_cmds.append("SETKEY IMAGETYP=%s\n" % imagetype)
        self.SA_cmds.append("SETKEY OBJECT=%s\n" % self.widgets["object_ET"].get_text())
        self.SA_cmds.append("SETKEY OBSERVER=%s\n" % self.widgets["observer_ET"].get_text())
        self.SA_cmds.append("EXPOSE %i %i\n" % (time, count))
        self.condition.release()

    def add_time_BT_clicked(self, widget):
        # TODO: implementovat
        pass

    def readout_BT_clicked(self, widget):
        if (self.window_question("Readout CCD?")): 
            self.action_readout = True

            self.condition.acquire()
            self.SA_cmds.append("READOUT\n")
            self.condition.release()
     
    def abort_BT_clicked(self, widget):
        if (self.window_question("Abort exposure?")): 
            self.action_abort = True

            self.condition.acquire()
            self.SA_cmds.append("ABORT\n")
            self.condition.release()

    def connect_BT_clicked(self, widget):
        self.sensitive_connects()

        self.disconnect = False
        username = self.widgets["username_ET"].get_text()
        password = self.widgets["password_ET"].get_text()

        self.condition.acquire()
        self.SA_cmds = []
        self.condition.release()

        self.client_TH = Thread(target = self.client_thread, args = (self.client_TH, username, password))
        self.client_TH.start()

        self.widgets["disconnect_BT"].set_sensitive(True)

    def disconnect_BT_clicked(self, widget):
        self.disconnect = True

    def execute_BT_clicked(self, widget):
        command = self.widgets["command_ET"].get_text()
        if (command != ""):
            self.condition.acquire()
            self.SA_cmds.append("%s\n" % command)
            self.condition.release()

    def path_OM_changed(self, widget):
        entry = self.widgets["%s_ET" % widget.get_name()[:-3]]
        position = widget.get_history()
        if (self.output_paths[position] == "other"):
            entry.set_text("")
            entry.set_editable(True)
        else:
            entry.set_text(self.output_paths[position])
            entry.set_editable(False)

    def show_ccdstate(self, state, elapsed, all):
        elapsed_pct = elapsed / (all / 100.0)
        elapsed_pct *= 0.01
        remain = all - elapsed
        hour_remain = remain / 3600
        remain = remain % 3600
        minute_remain = remain / 60
        second_remain = remain % 60

        self.widgets["state_LB"].set_markup("<b>%s</b>" % state)
        self.widgets["progressbar_LB"].set_text(_("Elapsed Time: %is" % elapsed))
        self.widgets["count_PB"].set_child_visible(True)
        self.widgets["count_PB"].set_text("%02i:%02i:%02i" % (hour_remain, minute_remain, second_remain))
        self.widgets["count_PB"].set_fraction(elapsed_pct)

    def get_ccdstate(self, data):
        exposing = patterns["exposing"].findall(data)
        reading_out_ccd = patterns["reading_out_ccd"].findall(data)
        ccd_is_ready = patterns["ccd_is_ready"].findall(data)

        # DBG
        #print "get_ccdstate = '%s'" % data

        if (exposing):
            self.sensitive_on_exposing()

            elapsed = int(exposing[0][0])
            all = int(exposing[0][1])
            filename = exposing[0][2]

            self.show_ccdstate("exposing", elapsed, all)
            self.widgets["file_ET"].set_text(filename)

            if ((all - elapsed) < 10):
                self.widgets["readout_BT"].set_sensitive(False)
        elif (reading_out_ccd):
            self.sensitive_on_exposing()
            self.widgets["readout_BT"].set_sensitive(False)

            elapsed = int(reading_out_ccd[0][0])
            all = int(reading_out_ccd[0][1])
            filename = reading_out_ccd[0][2]

            self.widgets["file_ET"].set_text(filename)
            self.show_ccdstate("reading out CCD", elapsed, all)
        elif (ccd_is_ready):
            filename = ccd_is_ready[0]

            self.sensitive_connects()
            self.widgets["file_ET"].set_text(filename)
            self.widgets["count_PB"].set_text("00:00:00")
            self.widgets["count_PB"].set_fraction(0)
            self.widgets["count_PB"].set_child_visible(False)
            self.widgets["progressbar_LB"].set_text('')
            self.widgets["state_LB"].set_markup("<b>ready</b>")

    def make_output_path_OM(self, data):
        paths = data.split(":")
        paths.append("other")
        self.output_paths = paths
        self.widgets["output_path_OM"].set_menu(values2menu(paths))
        self.widgets["output_path_OM"].set_history(0)
        self.widgets["output_path_ET"].set_text(paths[0])

    def make_archive_path_OM(self, data):
        paths = data.split(":")
        paths.append("other")
        self.archive_paths = paths
        self.widgets["archive_path_OM"].set_menu(values2menu(paths))
        self.widgets["archive_path_OM"].set_history(0)
        self.widgets["archive_path_ET"].set_text(paths[0])

    def instrument_LB_set_bold_text(self, text):
        self.widgets["instrument_LB"].set_markup("<b>%s</b>" % text)

    def log(self, typ, message):
        log_AJ = self.widgets["log_SW"].get_vadjustment()
        log_AJ.set_value(0.0)

        actual_time = time.gmtime()

        h = actual_time[3]
        m = actual_time[4]
        s = actual_time[5]

        message = "[%02i:%02i:%02i] %s" % (h, m, s, message)

        start = self.log_buffer.get_iter_at_offset(0)
        self.log_buffer.insert(start, message + "\n")
        stop = self.log_buffer.get_iter_at_offset(0)
        self.log_buffer.apply_tag_by_name(typ, start, stop)

    def poweroff(self, widget, event):
        if (self.window_question("Exit PESO")):
            self.action_quit = True
            return False
        else:
            return True
        
    def quit(self, window):
        gtk.main_quit()

    def client_thread(self, thread, username, password):
        if (thread != None):
            thread.join(3)

        pattern = None
        callback = None
        command = ""
        result = False
        auth = False
        wait_on_result = False
        ip = self.widgets["ip_ET"].get_text()
        port = int(self.widgets["port_ET"].get_text())

        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((ip, port))
        except Exception, e:
            gobject.idle_add(self.log, "error", e)
            return

        s.setblocking(0)

        # for Python 2.6
        # with self.condition:
        #     lock to synchronize access to some shared state

        self.condition.acquire()
        self.SA_cmds.append("USER %s\n" % username)
        self.SA_cmds.append("PASS %s\n" % password)
        self.SA_cmds.append("GET INSTRUMENT\n")
        self.SA_cmds.append("GET PATHS\n")
        self.SA_cmds.append("GET ARCHIVEPATHS\n")
        self.SA_cmds.append("GET CCDSTATE\n")
        self.condition.release()

        rlist = [s.fileno()]
        wlist = [s.fileno()]

        while ((not self.action_quit) and (not self.disconnect)):
            if (wait_on_result):
                wlist = []
            else:
                wlist = [s.fileno()]

            fdr, fdw, fde = select.select(rlist, wlist, [], 1)

            # read
            if (fdr != []):
                data = s.recv(1024)
                if (data == ""):
                    break

                # DBG
                #print "recv = '%s'" % data

                if (pattern):
                    value = pattern.findall(data)
                    if (value):
                        gobject.idle_add(callback, value[0])
                elif (callback):
                    gobject.idle_add(callback, data)
                else:
                    gobject.idle_add(self.log, "info", data.strip())

                if (data.find("+OK") != -1):
                    if (data.find("+OK EXPOSE") != -1):
                        gobject.idle_add(self.widgets["expose_BT"].set_sensitive, False)
                    result = True
                    wait_on_result = False
                    pattern, callback = [None, None]
                elif (data.find("-ERR") != -1):
                    result = False
                    wait_on_result = False
                    pattern, callback = [None, None]
                    self.condition.acquire()
                    self.SA_cmds = []
                    self.condition.release()

                if (not auth):
                    if ((not wait_on_result) and (result == True) and (command.find("PASS") != -1)):
                        auth = True
                    elif ((not wait_on_result) and (result == False)):
                        break

            # write
            if ((not wait_on_result) and (fdw != [])):
                command = ""
                self.condition.acquire()
                if (len(self.SA_cmds) > 0):
                    command = self.SA_cmds.pop(0)
                self.condition.release()

                if (command != ""):
                    # DBG
                    #print "send = '%s'" % command
                    s.send(command)
                    result = False

                    if (command.find("PASS") != -1):
                        gobject.idle_add(self.log, "command", "PASS ******")
                    elif (command.find("GET INSTRUMENT") != -1):
                        pattern = patterns["INSTRUMENT"]
                        callback = self.instrument_LB_set_bold_text
                    elif (command.find("GET PATHS") != -1):
                        pattern = patterns["PATHS"]
                        callback = self.make_output_path_OM
                    elif (command.find("GET ARCHIVEPATHS") != -1):
                        pattern = patterns["ARCHIVEPATHS"]
                        callback = self.make_archive_path_OM
                    elif (command.find("GET CCDSTATE") != -1):
                        pattern = None
                        callback = self.get_ccdstate
                    else:
                        gobject.idle_add(self.log, "command", command.strip())

                    wait_on_result = True
                else:
                    self.condition.acquire()
                    self.SA_cmds.append("GET CCDSTATE")
                    self.condition.release()

                    time.sleep(1)

        s.close()
        self.widgets["state_LB"].set_markup("<b>unknown</b>")
        gobject.idle_add(self.sensitive_disconnects)
        gobject.idle_add(self.widgets["main_NB"].set_current_page, 3)
        # DBG
        #print "close connection from %s:%i" % (ip, port)

def main():
    gdk.threads_init()
    peso = Peso()
    
if __name__ == "__main__":
    main()
