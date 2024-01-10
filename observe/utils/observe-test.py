#! /usr/bin/env python2.5
# -*- coding: utf-8 -*-

import sys
import time
import getpass
import signal
import httplib2

class Application():
    def __init__(self):
        self.exit = False
        self.count = 0
        self.start = time.time()

        signal.signal(signal.SIGINT, self.signal_handler)

        self.username = raw_input("Username: ")
        self.password = getpass.getpass()
        self.timeout = float(raw_input("Timeout: "))
        self.output = raw_input("Output [stdout]: ")

        self.http = httplib2.Http()
        self.http.add_credentials(self.username, self.password)
        
        self.headers = {"content-type": "application/xml"} 

        if (not self.output):
            self.out_fo = sys.stdout
        else:
            self.out_fo = open(self.output, "w")

        while (not self.exit):
            self.count += 1
            self.loop()
            time.sleep(self.timeout)

        self.out_fo.close()
        
    def loop(self):
        resp, content = self.http.request(
            "https://observe.asu.cas.cz/observe/telescope",
            "GET", headers = self.headers)
        
        elapsed_time = time.time() - self.start
        speed = float(self.count) / elapsed_time

        self.out_fo.write("%s\n" % resp)
        self.out_fo.write("%s\n" % content)
        self.out_fo.write("speed = %s requests/s, elapsed_time = %f, count requests = %d\n" %
            (speed, elapsed_time, self.count))

    def signal_handler(self, num, stack):
        print "Received signal %d, exiting..." % (num)
        self.exit = True

def main():
    application = Application()

if __name__ == "__main__":
    main()
