#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import errno

from datetime import datetime, timedelta

def mkdir_p(path):
    """ 'mkdir -p' in Python """
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if (exc.errno == errno.EEXIST) and os.path.isdir(path):
            pass
        else:
            raise

class RefreshSymlinkIncoming:

    def __init__(self):

        actual_night = datetime.now() - timedelta(hours=12)
        night_dir = actual_night.strftime("%Y/%Y%m%d")

        keys = ["incoming", "night"]
        directories = []
        directories.append(dict(zip(keys, ["/zfs/nfs/2m/CCD700.RAW/INCOMING", "/zfs/nfs/2m/CCD700.RAW/%s" % night_dir])))
        directories.append(dict(zip(keys, ["/zfs/nfs/2m/OES/INCOMING", "/zfs/nfs/2m/OES/%s" % night_dir])))

        # DBG
        #directories.append(dict(zip(keys, ["/tmp/zfs/nfs/2m/CCD700.RAW/INCOMING", "/tmp/zfs/nfs/2m/CCD700.RAW/%s" % night_dir])))
        #directories.append(dict(zip(keys, ["/tmp/zfs/nfs/2m/OES/INCOMING", "/tmp/zfs/nfs/2m/OES/%s" % night_dir])))

        if __debug__:
            print("actual_night = %s" % actual_night)
            print("directories = %s" % directories)

        for item in directories:
            mkdir_p(item["night"])

            if os.path.exists(item["incoming"]):
                if not os.path.islink(item["incoming"]):
                    raise Exception("'%s' is not symbolic link" % item["incoming"])
                elif os.path.realpath(item["incoming"]) != item["night"]:
                    if __debug__:
                        print("os.unlink", item["incoming"])
                    os.unlink(item["incoming"])
                else:
                    continue

            if __debug__:
                print("os.symlink", item["night"], item["incoming"])
            os.symlink(item["night"], item["incoming"])

def main():
    RefreshSymlinkIncoming()

if __name__ == '__main__':
    main()
