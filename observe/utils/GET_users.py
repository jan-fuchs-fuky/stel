#!/usr/bin/python

import os
import time
import subprocess

cmd = "curl --insecure -u fuky:hansjan -X GET https://localhost:8443/observe/users"

while (not os.path.isfile("/tmp/GET_users.stop")):
    #subprocess.call(cmd, shell=True)
    os.system("%s &>/dev/null &" % cmd)
    time.sleep(0.060)
