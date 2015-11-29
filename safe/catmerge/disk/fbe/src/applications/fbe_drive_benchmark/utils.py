#!/usr/bin/python

import time
from subprocess import call
from os import path, makedirs
import os
import re
import platform

FBECLI_log_directory = "cli_log"

def is_in_windows():
    sys_info = platform.system().lower()
    if sys_info == "windows":
        return True

def time_to_string(tm):
    return "%u-%02u-%-2u %02u:%02u:%02u" % (
        tm.tm_year, tm.tm_mon, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec)

def time_to_filename(tm):
    return "%u%02u%02u_%02u%02u%02u" % (
        tm.tm_year, tm.tm_mon, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec)

def get_work_dir(cfg, st):
    test_dir = "test_" + time_to_filename(st)
    return cfg.base_directory + test_dir

def get_fbe_command(cfg):
    fbecli = "fbecli.exe"
    if cfg.sim:
        fbecli += " s a"
    return fbecli

def get_output_filename(input_filename):
    name, ext = path.splitext(input_filename)
    output_filename = name + "_out" + ext
    return output_filename

def get_script_filename(name):
    input_name = path.join(FBECLI_log_directory, name)
    output_name = get_output_filename(input_name)
    return input_name, output_name

def run_command(cmd, cfg=None):
    print "Run command: %s" % cmd
    if cfg and cfg.verbose:
        output = None
    #elif re.findall("rba", cmd):
        #output = None
    else:
        output = open(os.devnull, "w")
    ret = call(cmd, stdout=output, stderr=output, shell=True)
    if output:
        output.close()
    return ret

def run_fbecli(cfg, cmd, input_filename, output_filename=None):
    with open(input_filename, "w") as f:
        f.write(cmd)
    if not output_filename:
        output_filename = get_output_filename(input_filename)
    fbecli_cmd = "%s z%s o%s" % (get_fbe_command(cfg), input_filename, output_filename)
    ret = run_command(fbecli_cmd, cfg)
    if ret != 0:
        raise Exception("Run cmd %s failed" % fbecli_cmd)


def get_rba_file(bm):
    bm_name = bm.get_name()
    #bm_name = bm_name[:15]
    return path.join(os.getcwd(), bm_name + ".ktr")


def start_neit():
    if is_in_windows():
        start_neit_command = "net start newneitpackage"
        run_command(start_neit_command)


def stop_rba(cfg, bm):
    rbafile = get_rba_file(bm)
    stop_rba_command = "rba.exe -c -t off"
    if cfg.sim:
        with open(rbafile, "a") as f:
            f.write(stop_rba_command + "\n")
    else:
        run_command(stop_rba_command)


def start_rba(cfg, bm):
    rbafile = get_rba_file(bm)
    start_cmd = "rba.exe -o %s -t pdo -t pvd" % rbafile
    #priority_cmd = "rba.exe -j 15"
    if cfg.sim:
        with open(rbafile, "a") as f:
            f.write(start_cmd + "\n")
            #f.write(priority_cmd + "\n")
    else:
        run_command(start_cmd)
        #run_command(priority_cmd)


def list_unique(l):
    seen = set()
    seen_add = seen.add
    return [x for x in l if x not in seen and not seen_add(x)]


class TestError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return str(self.value)


class tee_writer(object):
    def __init__(self, *files):
        self.files = files

    def write(self, txt):
        for fp in self.files:
            fp.write(txt)
            fp.flush()

    def flush(self):
        for fp in self.files:
            fp.flush()

