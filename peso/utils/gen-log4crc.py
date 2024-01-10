#!/usr/bin/python2.5
# -*- coding: utf-8 -*-

template = "../etc/log4crc.template.xml"
output = "../etc/log4crc.%(ccd)s"

ccds = [
    "sauron",
    "frodo",
    "bilbo",
    "gandalf",
]

log4crc = {}
log4crc["path"] = "/opt/exposed/log"
log4crc["prefix"] = "exposed"
log4crc["ccd"] = ""

fo = open(template, "r")
data = fo.read()
fo.close

for ccd in ccds:
    log4crc["ccd"] = ccd

    fo = open(output % log4crc, "w")
    fo.write(data % log4crc)
    fo.close()
