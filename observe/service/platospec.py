#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import traceback

from invoke import Exit
from fabric import task

OUTPUT_DIR = "./_tmp"

linux1_hosts = ["platospec-linux1.stel"]
workstation_hosts = ["platospec-workstation.stel"]

camera_client_file_lst = [
    "/opt/fiber_pointing_client/bin/fiber_pointing_client.py",
    "/opt/fiber_pointing_client/etc/fiber_pointing_client.cfg",
    "/opt/fiber_pointing_client/share/fiber_pointing_client.ui",
]

def parse_filename(c, item):
    subdir = os.path.basename(os.path.dirname(item))
    filename = os.path.basename(item)
    output_dir = os.path.join(OUTPUT_DIR, c.original_host, subdir)
    dst = os.path.join(output_dir, filename)
    local_filename = os.path.join("./", subdir, filename)

    return [dst, output_dir, local_filename]

def get_file(c, item):
    dst, output_dir, _ = parse_filename(c, item)

    os.makedirs(output_dir, exist_ok=True)

    print("get %s %s" % (item, dst))
    c.get(item, dst)

def diff_file(c, item):
    dst, _, local_filename = parse_filename(c, item)
    diff_filename = "%s.diff" % dst

    # WARNING: nesmi se pouzit >, protoze jinak skonci zavolani prikazu:
    #     invoke.exceptions.UnexpectedExit: Encountered a bad command exit code!
    cmd = "diff --color=always %s %s |tee %s" % (dst, local_filename, diff_filename)

    # TODO: reagovat na vyjimku
    print(cmd)
    result = c.local(cmd)

@task(hosts=linux1_hosts)
def get_linux1(c):
    for item in camera_client_file_lst:
        get_file(c, item)
        diff_file(c, item)

@task(hosts=linux1_hosts)
def diff_linux1(c):
    for item in camera_client_file_lst:
        print(item)
        diff_file(c, item)

@task(hosts=linux1_hosts)
def deploy_linux1(c):
    c.run("uptime")
