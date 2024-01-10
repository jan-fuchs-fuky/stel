#!/usr/bin/python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#

import os
import sys
import time
import argparse
import signal
import traceback
import xmlrpc.client

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

EXPOSE_BEFORE_AFTER_CMD_TMPDIR = "%s/../temp/" % SCRIPT_PATH

class ExposeBeforeAfterCmd:

    def __init__(self):

        signal.signal(signal.SIGALRM, self.handler)

        self.toptec_proxy = xmlrpc.client.ServerProxy("http://192.168.193.198:6000")
        self.quido_proxy = xmlrpc.client.ServerProxy("http://192.168.193.198:6001")

        #self.toptec_proxy = xmlrpc.client.ServerProxy("http://localhost:6000")
        #self.quido_proxy = xmlrpc.client.ServerProxy("http://localhost:6001")

        # relay_number, camera_position
        self.setup = {
            "comp": [1, 14300],
            "flat": [2, 15600],
        }

    def run(self, action, value):
        signal.alarm(60)

        self.run_setup(action, value)

        signal.alarm(0)
        print("EXIT")
        sys.exit(0)

    def run_setup(self, action, value):
        relay_number, camera_position = self.setup[action]

        toptec_values = self.toptec_proxy.toptec_get_values()

        filename = os.path.join(EXPOSE_BEFORE_AFTER_CMD_TMPDIR, "expose_before_after_cmd.txt")

        if value == 1:
            with open(filename, "w") as fo:
                fo.write("%i\n" % toptec_values["cameras_position"])
        else:
            if os.path.isfile(filename):
                with open(filename, "r") as fo:
                    camera_position = int(fo.read().strip())

        self.quido_proxy.quido_set_relay(relay_number, value)

        self.toptec_proxy.toptec_set_camera_position(camera_position)

        while abs(toptec_values["cameras_position"] - camera_position) > 1:
            time.sleep(1)
            toptec_values = self.toptec_proxy.toptec_get_values()

    def handler(self, signum, frame):
        print("Timeout")
        sys.exit(1)

def main():
    epilog = (
        ""
    )

    parser = argparse.ArgumentParser(
        description="Run command before or after exposure.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=epilog)

    parser.add_argument("-f", "--flat", type=int, default=None, metavar="VALUE",
                        choices={0, 1},
                        help="VALUE is integer 0 (OFF) or 1 (ON)")

    parser.add_argument("-c", "--comp", type=int, default=None, metavar="VALUE",
                        choices={0, 1},
                        help="VALUE is integer 0 (OFF) or 1 (ON)")

    args = parser.parse_args()

    client = ExposeBeforeAfterCmd()

    if args.flat is not None:
        client.run("flat", args.flat)
    if args.comp is not None:
        client.run("comp", args.comp)

if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        err_msg = " ".join(traceback.format_exception_only(exc_type, exc_value)).strip()
        print(err_msg)
        sys.exit(1)
