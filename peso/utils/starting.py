#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
#
# $Date$
# $Rev$
# $URL$
#

import sys
import gtk
import gobject
import subprocess
import time
import traceback
import threading

class Starting():
    
    def __init__(self, pid, message):
        self.exit = False
        self.pid = pid
        self.message = message

        gobject.threads_init()

        window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        window.set_title("Starting")
        window.set_position(gtk.WIN_POS_CENTER)
        window.set_size_request(360, 270)
        window.set_resizable(False)
        window.connect("delete-event", self.quit)

        vbox = gtk.VBox()
        vbox.show()

        self.label = gtk.Label(self.message)
        self.label.show()
        vbox.pack_start(self.label, expand=True, fill=True, padding=0)

        self.progress_bar = gtk.ProgressBar()
        self.progress_bar.show()
        vbox.pack_start(self.progress_bar, expand=False, fill=True, padding=0)

        window.add(vbox)
        window.show()

        wait_TH = threading.Thread(target = self.wait_thread)
        wait_TH.start()

        gtk.main()

    def wait_thread(self):
        while (not self.exit):
            retcode = subprocess.call(["ps", "-p", self.pid, "-o", "comm="])
            if (retcode != 0):
                gobject.idle_add(gtk.main_quit)
                break

            gobject.idle_add(self.progress_bar.pulse)
            time.sleep(0.1)

    def quit(self, a, b):
        self.exit = True
        gtk.main_quit()

def main():
    if (len(sys.argv) != 3):
        print "Usage: %s PID MESSAGE" % sys.argv[0]
        sys.exit(0)

    Starting(sys.argv[1], sys.argv[2])

if __name__ =='__main__':
    main()
