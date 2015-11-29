#!/usr/bin/python

from __future__ import print_function
import sys
from os import path


BULLSEYE_TEMPLATE_FILE = "bullseye_template.cfg"
BULLSEYE_SCRIPT_PATH = "safe/catmerge/"


def error(*args):
    print("ERR    ", *args, file=sys.stderr)

def info(*args):
    print("INFO   ", *args, file=sys.stderr)


def gen_exclude(line, prefix):
    line = line.strip()
    if not line:
        return
    files = line.split()
    if len(files) != 3:
        error("Skip ", line)
        return
    element = files[2]
    path = prefix + element
    if files[0] == "include":
        print("-t%s" % path)
    elif files[0] == "exclude":
        print("-t!%s" % path)
    else:
        error("Unknow operation <%s>, skip %s" % (files[0], line))


def main(template, prefix):
    inputs = None
    with open(template) as f:
        inputs = f.readlines()
    print("-a\n-v")
    for line in inputs:
        gen_exclude(line, prefix)


def guess_template_file():
    script_dir = path.dirname(__file__)
    template_file = path.join(script_dir, BULLSEYE_TEMPLATE_FILE)
    if path.isfile(template_file):
        info("Using template file %s" % template_file)
        return template_file
    return ""


def guess_workspace():
    script_dir = path.dirname(path.abspath(__file__))
    find_pos = script_dir.find(BULLSEYE_SCRIPT_PATH)
    if find_pos != -1:
        workspace = script_dir[:find_pos]
        info("Using workspace %s" % workspace)
        return workspace
    return ""


def usage():
    error("Usage: %s template_file [/path/to/workspace]" % sys.argv[0])
    sys.exit(1)


if __name__ == "__main__":
    #example: ./gencfg.py bullseye_template.cfg /home/c4dev/code/unity_int_code/ > ~/bullseye.cfg
    template_file = ""
    workspace = ""
    prefix = ""
    if len(sys.argv) < 2:
        template_file = guess_template_file()
    else:
        template_file = sys.argv[1]

    if not template_file:
        error("Can't find template file")
        usage()


    if len(sys.argv) < 3:
        workspace = guess_workspace()
    else:
        workspace = sys.argv[2]

    if not workspace:
        error("Can't find workspace")
        usage()

    prefix = path.join(workspace, "safe/")
    main(template_file, prefix)
