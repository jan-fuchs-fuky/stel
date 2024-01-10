#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
#
# $Date$
# $Rev$
# $URL$
#

import os
import sys
import cmd
import signal
import readline
import atexit
import time
import curses
import traceback
import xmlrpclib

def restore_screen():
    curses.nocbreak()
    curses.echo()
    curses.endwin()

class ObserveShell(cmd.Cmd):

    def preloop(self):
        self.screen = curses.initscr()
        restore_screen()

        signal.signal(signal.SIGINT, signal.SIG_IGN) # Ctrl+C

        self.ccd = "oes"
        self.ccd_names = ["oes", "700", "400"]

        self.expose_info = {
            "archive": 0,
            "archive_path": "",
            "archive_paths": "",
            "ccd_temp": 0,
            "elapsed_time": 0,
            "filename": "",
            "full_time": 0,
            "instrument": "",
            "path": "",
            "paths": "",
            "state": "",
        }

        self.header = ""
        str = []
        str.append("\n")
        str.append("CCD state: %(ccd_state)12s, Expose number/count: %(expose_number)s/%(expose_count)s\n")
        str.append(" CCD temp: %(ccd_temp)12s, Readout speed: %(ccd_speed)s\n")
        str.append(" Filename: %(filename)12s, Data path: %(data_path)s\n")
        str.append("   Object: %(object)12s, Observers: %(observer)s\n")
        self.header_format = "".join(str)

        self.oes = xmlrpclib.ServerProxy("http://192.168.2.1:5000")
        self.run_expose_info()

        self.keys = {}
        self.keys["OBSERVER"] = ""
        self.keys["OBJECT"] = ""
        self.keys["IMAGETYP"] = ""

        self.variables = {}
        self.variables["PATH"] = self.expose_info["paths"].split(";")[0]
        self.variables["READOUT_SPEED"] = "1MHz"

        self.prompt_refresh()

        if (self.expose_info["state"] != "ready"):
            self.expose_loop()

    def header_refresh(self):
        self.run_expose_info()

        header_values = {}
        header_values["ccd"] = self.ccd
        header_values["ccd_state"] = self.expose_info["state"]
        header_values["filename"] = self.expose_info["filename"]
        header_values["data_path"] = self.variables["PATH"]
        header_values["ccd_temp"] = self.expose_info["ccd_temp"]
        header_values["ccd_speed"] = self.variables["READOUT_SPEED"]
        header_values["observer"] = self.keys["OBSERVER"]
        header_values["object"] = self.keys["OBJECT"]
        header_values["expose_number"] = self.expose_info["expose_number"]
        header_values["expose_count"] = self.expose_info["expose_count"]

        self.header = self.header_format % header_values

    def prompt_refresh(self):
        self.header_refresh()
        self.prompt = "%s\n%s> " % (self.header, self.ccd)

    #def signal_handler(self, signum, frame):
    #    # Ctrl+C
    #    if (signum == signal.SIGINT):
    #        print "\nIgnoring Ctrl+C"

    def print_exc(self):
        print "*** ERROR: %s" % traceback.format_exc().splitlines()[-1]

    def run_expose_info(self):
        try:
            self.expose_info = self.oes.expose_info()
        except:
            self.print_exc()

    def run_expose_abort(self):
        try:
            self.oes.expose_abort()
        except:
            self.print_exc()

    def run_expose_readout(self):
        try:
            self.oes.expose_readout()
        except:
            self.print_exc()

    def run_expose_set_key(self, key, value, comment=""):
        try:
            result = self.oes.expose_set_key(key, value, comment)
        except:
            self.print_exc()
            return

        if (not result.startswith("+OK")):
            print "*** WARNING: set_key %s = %s => %s" % (key, value, result)

    def run_expose_set(self, variable, value):
        try:
            result = self.oes.expose_set(variable, value)
        except:
            seld.print_exc()
            return

        if (not result.startswith("+OK")):
            print "*** WARNING: set %s = %s => %s" % (variable, value, result)

    def prepare_expose(self, args, imagetyp):
        try:
            seconds, count = [ int(s) for s in args.split() ]
        except:
            self.print_exc()
            return

        if (imagetyp == "zero"):
            self.keys["OBJECT"] = "zero"
        elif (imagetyp == "dark"):
            self.keys["OBJECT"] = "dark"
        elif (imagetyp == "flat"):
            self.keys["OBJECT"] = "flat"
        elif (imagetyp == "comp"):
            self.keys["OBJECT"] = "comp"

        self.keys["IMAGETYP"] = imagetyp

        self.expose(seconds, count)
        self.prompt_refresh()

    def do_EOF(self, args):
        return True

    def do_exit(self, args):
        return True

    def do_quit(self, args):
        return True

    def help_refresh(self):
        print "refresh: Refresh information"
        print "Usage: refresh"

    def do_refresh(self, args):
        self.prompt_refresh()

    # TARGET
    def help_exp(self):
        print "exp: Run exposure.\n"
        print "Usage: exp [SECONDS]"

    def do_exp(self, time):
        self.prepare_expose("%s 1" % seconds, "target")

    def help_mexp(self):
        print "mexp: Run multiple exposures.\n"
        print "Usage: exp SECONDS COUNT"

    def do_mexp(self, args):
        self.prepare_expose(args, "target")

    # ZERO
    def help_zero(self):
        print "exp: Run zero.\n"
        print "Usage: zero [SECONDS]"

    def do_zero(self, seconds):
        self.prepare_expose("%s 1" % seconds, "zero")

    def help_mzero(self):
        print "mzero: Run multiple zeros.\n"
        print "Usage: mzero SECONDS COUNT"

    def do_mzero(self, args):
        self.prepare_expose(args, "zero")
    
    # DARK
    def help_dark(self):
        print "exp: Run dark.\n"
        print "Usage: dark [SECONDS]"

    def do_dark(self, seconds):
        self.prepare_expose("%s 1" % seconds, "dark")

    def help_mdark(self):
        print "mdark: Run multiple darks.\n"
        print "Usage: mdark SECONDS COUNT"

    def do_mdark(self, args):
        self.prepare_expose(args, "dark")

    # FLAT
    def help_flat(self):
        print "exp: Run flat.\n"
        print "Usage: flat [SECONDS]"

    def do_flat(self, seconds):
        self.prepare_expose("%s 1" % seconds, "flat")

    def help_mflat(self):
        print "mflat: Run multiple flats.\n"
        print "Usage: mflat SECONDS COUNT"

    def do_mflat(self, args):
        self.prepare_expose(args, "flat")

    # COMP
    def help_comp(self):
        print "exp: Run comp.\n"
        print "Usage: comp [SECONDS]"

    def do_comp(self, seconds):
        self.prepare_expose("%s 1" % seconds, "comp")

    def help_mcomp(self):
        print "mcomp: Run multiple comps.\n"
        print "Usage: mcomp SECONDS COUNT"

    def do_mcomp(self, args):
        self.prepare_expose(args, "comp")

    # CCD
    def help_ccd(self):
        print "ccd: Select CCD.\n"
        print "Usage: ccd [NAME]\n"
        print "Valid NAME is %s"  % " or ".join(self.ccd_names)

    def do_ccd(self, name):
        if (name in self.ccd_names):
            self.ccd = name
            self.prompt_refresh()
        else:
            print "Please specify the name of the CCD (%s)" % ", ".join(self.ccd_names)

    # IMPATH
    def help_impath(self):
        print "impath: Change output path.\n"
        print "Usage: impath\n"

    def do_impath(self, path):
        paths = self.expose_info["paths"].split(";")

        idxs = []
        idx = 0
        for path in paths:
            print "%i) %s" % (idx, path)
            idxs.append(str(idx))
            idx += 1

        result = int(raw_input("Select output path %s: " % "/".join(idxs)))
        if (len(paths) <= result) or (result < 0):
            print "*** ERROR: %s is not a valid option" % result
        else:
            self.variables["PATH"] = paths[result]

        self.prompt_refresh()

    # OBJECT
    def help_object(self):
        print "object: Change object name.\n"
        print "Usage: object NAME"

    def do_object(self, name):
        self.keys["OBJECT"] = name
        self.prompt_refresh()

    # OBSERVER
    def help_observer(self):
        print "observer: Change observer name.\n"
        print "Usage: observer NAME"

    def do_observer(self, name):
        self.keys["OBSERVER"] = name
        self.prompt_refresh()

    def default(self, line):
        print "Unknown command: %s" % line

    def show_question(self, question):
        self.screen.addstr("\n")
        self.screen.addstr(question)
        self.screen.refresh()

        while (True):
            try:
                c = chr(self.screen.getch())
            except:
                continue

            self.screen.deleteln()

            if (c == "y"):
                return True
            else:
                return False

    def sec2human(self, sec):
        h = sec / 3600
        sec = sec % 3600
        m = sec / 60
        sec = sec % 60

        return "%02i:%02i:%02i" % (h, m, sec)

    def expose_info2human_time(self, info):
        elapsed = int(info["elapsed_time"])
        full = int(info["full_time"])
        remained = full - elapsed

        time = {}
        time["elapsed"] = self.sec2human(elapsed)
        time["full"] = self.sec2human(full)
        time["remained"] = self.sec2human(remained)

        return time

    def expose_loop(self):
        curses.noecho()
        curses.cbreak()
        self.screen.clear()
        self.screen.refresh()
        self.screen.timeout(200)

        abort = False
        readout = False

        while (True):
            self.header_refresh()
            if (self.expose_info["state"] == "ready"):
                break

            self.screen.clear()
            self.screen.addstr(self.header)
            self.screen.addstr("\nElapsed %(elapsed)s, remained %(remained)s from %(full)s\n\n" % \
                self.expose_info2human_time(self.expose_info))
            self.screen.addstr("Press 'a' for abort, 'r' for readout\n")
            self.screen.refresh()

            if (abort):
                self.run_expose_abort()
            elif (readout):
                self.run_expose_readout()

            try:
                c = chr(self.screen.getch())
            except:
                continue

            if (c == "a"):
                abort = self.show_question("Abort y/[n]: ")
            elif (c == "r"):
                readout = self.show_question("Readout y/[n]: ")
            elif (c == "c"):
                pass

        restore_screen()

    def expose(self, seconds, count):
        if (not self.keys["OBSERVER"]):
            print "*** ERROR: Please specify observer name"
            return False
            
        if (not self.keys["OBJECT"]):
            print "*** ERROR: Please specify object name"
            return False

        self.run_expose_set_key("OBJECT", self.keys["OBJECT"])
        self.run_expose_set_key("OBSERVER", self.keys["OBSERVER"])
        self.run_expose_set_key("IMAGETYP", self.keys["IMAGETYP"])
        self.run_expose_set("READOUT_SPEED", self.variables["READOUT_SPEED"])
        self.run_expose_set("PATH", self.variables["PATH"])

        try:
            result = self.oes.expose_start(seconds, count)
        except:
            print "*** ERROR: %s" % traceback.format_exc().splitlines()[-1]
            return False

        if (not result.startswith("+OK")):
            print result
            return False

        self.expose_loop()

def main():
    histfile = os.path.expanduser("~/.observe_history")
    
    try:
        readline.read_history_file(histfile)
    except IOError:
        pass
    
    atexit.register(readline.write_history_file, histfile)

    observe_shell = ObserveShell()
    observe_shell.cmdloop("Observe command-line interface")

if __name__ =='__main__':
    try:
        main()
    except:
        restore_screen()
        traceback.print_exc()
