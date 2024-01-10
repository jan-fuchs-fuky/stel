#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import traceback
import xmlrpc.client

from datetime import datetime

def main():
    proxy = xmlrpc.client.ServerProxy("http://127.0.0.1:6001")

    while True:
        dt_begin = datetime.utcnow()
        counter = 0

        while True:
            counter += 1

            try:
                print(proxy.quido_get_relays())
                print(proxy.quido_get_temperature())
            except:
                traceback.print_exc()
                print("waiting 10s...")
                time.sleep(10)

            if (datetime.utcnow() - dt_begin).total_seconds() > 60:
                print("counter = %i, waiting 1s..." % counter)
                time.sleep(1)
                break

if __name__ == '__main__':
    main()
