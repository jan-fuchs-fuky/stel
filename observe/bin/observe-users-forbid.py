#! /usr/bin/env python
# -*- coding: utf-8 -*-

#
# $Date: 2011-02-23 16:35:36 +0100 (Wed, 23 Feb 2011) $
# $Rev: 284 $
# $URL: https://stelweb.asu.cas.cz/svn/observe/trunk/bin/observe-users-forbid.py $
#

import os
import sys
import ssl
import urllib
import urllib2
import logging
import ConfigParser

observe_dir = "/opt/observed"
if (os.environ.has_key("OBSERVE_DIR")):
    observe_dir = os.environ.get("OBSERVE_DIR")

def main():
    cfg = ConfigParser.RawConfigParser()
    cfg.read("%s/etc/observe-users-forbid.cfg" % observe_dir)
    
    realm = cfg.get("observe", "realm")
    uri = cfg.get("observe", "uri")
    user = cfg.get("observe", "user")
    password = cfg.get("observe", "password")
    url = cfg.get("observe", "url")
    
    auth_handler = urllib2.HTTPBasicAuthHandler()
    auth_handler.add_password(realm, uri, user, password)

    context = ssl._create_unverified_context()
    ssl_unverified_context_handler = urllib2.HTTPSHandler(context=context)

    opener = urllib2.build_opener(ssl_unverified_context_handler, auth_handler)
    urllib2.install_opener(opener)
    
    params = {}
    params.update({"login": "all"})
    params.update({"permission": "none"})
    params.update({"password": ""})
    
    result = urllib2.urlopen(url, urllib.urlencode(params))

    if (result.code != 200):
        raise Exception("%s %s" % (result.code, result.msg))

if __name__ == '__main__':
    logger = logging.getLogger("observe-users-forbid")
    logger.setLevel(logging.DEBUG)

    formatter = logging.Formatter("%(asctime)s - %(name)s[%(process)d] %(threadName)s[%(thread)d] - %(levelname)s - %(message)s")
    fh = logging.FileHandler("%s/log/observe-users-forbid.log" % observe_dir)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)
    logger.addHandler(fh)

    try:
        main()
    except Exception:
        logger.exception("exception")
        sys.exit(1)

    logger.info("success")
    sys.exit(0)
