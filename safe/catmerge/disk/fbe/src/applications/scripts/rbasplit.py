#!/usr/bin/python

import sys
from rbadef import *
import os
from os import path
import subprocess

file_size_mb = 100
ktrsplit_name = "ktrsplit.jar"

def do_split(bin_file, file_name, mb):
    cmd = "java -jar %s -i %s -t mb -s %u -o" % (bin_file, file_name, mb)
    print "Running", cmd, "for splitting"
    status = subprocess.call(cmd, shell=True)
    return status

def gen_split_rba_name(orig_name, idx):
    base,ext = path.splitext(orig_name)
    return "%s_%u%s" % (base, idx, ext)

def format_filename(input_file, gm):
    base,ext = path.splitext(input_file)
    return "%s--%u%02u%02u-%02u%02u%02u%s" % (base, gm.tm_year, gm.tm_mon, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec, ext)

def get_file_start_time(file_name):
    f = open(file_name, "rb")

    # read header to calculate boot time and rtc_freq
    header_data = f.read(512)
    info = gen_header_dict(header_data)
    #print info

    record_size = calcsize(record_format)
    first_record = f.read(record_size)
    if len(first_record) < record_size:
        print "Wrong file length for '%u'" % file_name
        return None
    cur_time = get_record_time_by_header(info, first_record)
    cur_utc = to_utc_time(cur_time)
    return cur_utc

def rba_rename(file_name):
    cur_utc = get_file_start_time(file_name)
    name = format_filename(file_name, cur_utc[0])
    try:
        if path.exists(name):
            print "WARNING: overwrite %s" % name
            os.unlink(name)
        os.rename(file_name, name)
    except IOError:
        print "Rename failed"

    return name

def rba_split_file(bin_path, file_name):
    #invoke ktrsplit to split the file
    status = do_split(bin_path, file_name, file_size_mb)
    if status != 0:
        print "Split failed"
        return None

    splitted_files = []
    for i in range(1, 22):
        new_name = gen_split_rba_name(file_name, i)
        if path.exists(new_name):
            splitted_files.append(new_name)

    new_files = []
    for f in splitted_files:
        name = rba_rename(f)
        print "%s -> %s" % (f, name)
        new_files.append(name)
    return new_files

if __name__ == "__main__":
    #TODO: Add command line parse here
    if len(sys.argv) < 2:
        print "Usage: %s rbafile" % sys.argv[0]
        sys.exit(1)

    bin_dir = path.dirname(sys.argv[0])
    if bin_dir == "":
        ktrsplit_bin_path = "ktrsplit.jar"
    else:
        ktrsplit_bin_path = bin_dir + "/" + "ktrsplit.jar"

    new_files = rba_split_file(ktrsplit_bin_path, sys.argv[1])
    if new_files:
        print new_files
        print "Splitted done"
    else:
        print "Splitted failed"
