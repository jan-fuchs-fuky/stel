#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# $ openssl genrsa -out privkey_test.pem 2048
# $ openssl req -new -x509 -days 3650 -key privkey_test.pem -out cert_test.pem
#

import sys
sys.stdout = sys.stderr

import os
import time
import cherrypy
import configparser
import hashlib

from cherrypy.lib import auth_basic
from cherrypy.process.plugins import Daemonizer, PIDFile

SCRIPT_PATH = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
sys.path.append(SCRIPT_PATH)

def check_password(realm, user, password):
    user_passwd = ("%s:%s" % (user, password)).encode("utf-8")
    hexdigest = hashlib.sha512(user_passwd).hexdigest()

    if (hexdigest == "0a4633d1707e79ebe30152041ecceea77e2fd2850c6ea27cec17ce18d8e87b3fbe0f1dbed7d08b8c22075ab3c27cd9d110f46ba59fb3734c20f1b9e08704baa9"):
        return True
    else:
        return False

class ObserveServerTest(object):

    @cherrypy.expose
    def index(self):
        return "Test Observe Server"

    @cherrypy.expose
    def telescope(self):
        xml = "<telescope_info><dopo>4.37</dopo><fopo>42.35</fopo><glst>0 1 0 0 0 1 4 4 4 511 0 0 0 0</glst><object>unknown</object><trcs>3</trcs><trgv>-12.6 -2.4</trgv><trhd>85.9603 39.2634</trhd><trrd>210016.153 +390618.20 0</trrd><trus>0.0000 0.0000</trus><tsra>064614.15 793353.32 0</tsra><ut>2018-03-07 14:44:30</ut></telescope_info>"
        return xml

    @cherrypy.expose
    def spectrograph(self):
        xml = "<spectrograph_info><glst>2 1 2 0 0 1 2 0 0 2 1 1 0 0 1 1 2 0 0 0 2 0 2 0 0 2 0 0</glst><spce_14>0</spce_14><spce_24>0</spce_24><spfe_14>0</spfe_14><spfe_24>0</spfe_24><spgp_13>6457</spgp_13><spgp_22>3080</spgp_22><spgp_4>4900</spgp_4><spgp_5>4020</spgp_5><spgs_19>17636</spgs_19><spgs_20>17432</spgs_20></spectrograph_info>"
        return xml

    @cherrypy.expose
    def oes(self):
        xml = "<expose_info><archive>1</archive><archive_path/><archive_paths/><ccd_temp>-110.0</ccd_temp><elapsed_time>42</elapsed_time><expose_count>2</expose_count><expose_number>2</expose_number><filename>c201803060024.fit</filename><full_time>0</full_time><instrument>OES</instrument><path>/data/OES/INCOMING</path><paths>/data/OES/INCOMING;/data/OES/INCOMING/TECH;/tmp</paths><state>ready</state></expose_info>"
        return xml

    @cherrypy.expose
    def ccd700(self):
        xml = "<expose_info><archive>1</archive><archive_path/><archive_paths/><ccd_temp>-115.0</ccd_temp><elapsed_time>19</elapsed_time><expose_count>5</expose_count><expose_number>5</expose_number><filename>a201803010045.fit</filename><full_time>0</full_time><instrument>CCD700</instrument><path>/data/CCD700.RAW/INCOMING</path><paths>/data/CCD700.RAW/INCOMING;/data/CCD700.RAW/INCOMING/TECH;/tmp</paths><state>ready</state></expose_info>"
        return xml

    @cherrypy.expose
    def ccd400(self):
        xml = "<expose_info><archive>0</archive><archive_path>pleione:/tmp</archive_path><archive_paths/><ccd_temp>-100.5</ccd_temp><elapsed_time>0</elapsed_time><expose_count>1</expose_count><expose_number>1</expose_number><filename/><full_time>0</full_time><instrument>CCD400</instrument><path>/tmp</path><paths>/data/CCD400.RAW/INCOMING;/data/CCD400.RAW/INCOMING/TECH;/tmp</paths><state>ready</state></expose_info>"
        return xml

    @cherrypy.expose
    def users(self):
        xml = "<user><email>fuky@asu.cas.cz</email><firstName>test</firstName><lastName>test</lastName><login>test</login><permission>control</permission></user>"
        return xml

def main():
    #d = Daemonizer(cherrypy.engine)
    #d.subscribe()

    PIDFile(cherrypy.engine, "%s/run/observe-server.pid" % SCRIPT_PATH).subscribe()

    conf_file = "%s/observe-server.cfg" % SCRIPT_PATH

    conf = {
        "/": {
            "tools.auth_basic.checkpassword": check_password
        }
    }

    cherrypy.config.update(conf_file)
    cherrypy.config.update(conf)

    app = cherrypy.tree.mount(ObserveServerTest(), config=conf_file)
    app.merge(conf)

    cherrypy.engine.signals.subscribe()
    cherrypy.engine.start()
    cherrypy.engine.block()

if __name__ == '__main__':
    main()
