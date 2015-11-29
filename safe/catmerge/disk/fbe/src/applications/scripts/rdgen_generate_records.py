###########################################################################
# Copyright (C) EMC Corporation 2013
# All rights reserved.
# Licensed material -- property of EMC Corporation
##########################################################################
  
##########################################################################
# rdgen_generate_records.py
# 
# The purpose of this tool is to generate an rdgen file that has
# an artificial workload.
# 
# This script is used to generate a file that can be used to
# test rdgen.
# 
##########################################################################
import sys
import os
import re
from struct import *
from rba_to_rdgen import *

class app_control:
    def __init__(self):
        self.input_file_name = ""
        self.output_file_name = ""
        self._script_name = "rdgen_generate_records_rdg.py"

    def usage(self, script_name):
        print ''

    def process_args(self):
        if len(sys.argv) < 2:
            self.usage(sys.argv[0])
            print "%s expects a config file path name" % self._script_name
            return False
        self.input_file_name = sys.argv[1]
        if len(sys.argv) < 2:
            self.usage(sys.argv[0])
            print "%s expects a file name for output files" % self._script_name
            return False
        self.output_file_name = sys.argv[2]
        return True

if __name__ == "__main__":
    app = app_control()

    if (app.process_args() == False):
        exit(0)

    print("output file is: " + app.output_file_name)

    config_file = rdgen_config_file(app.input_file_name)
    config_file.read_file()

    package_id = "sep"

    print config_file._mapping_dict.keys()
    rec_list = []
    total_chunks = 500
    
    for i in range(0, 819):
        if (i % 400) == 0: # was %400
            threads = 0
            ramp_msecs = 200
        else:
            threads = config_file._threads
            ramp_msecs = 0
        rec_dict = {
             'opcode' : 1,
            'lba' : 0x50+i,
            'blocks' : 10,
            'core' : (0 + i) % 4,
            'flags' : i+0x100,  
            #'threads' : (i % config_file._threads) + 1,
            'threads' :  threads,
            'ramp_msecs' : ramp_msecs }
        #print rec_dict
        rec_list.append(rdgen_record(None, rec_dict))
    for i in range(0, 819):
        rec_dict = {
             'opcode' : 2,
            'lba' : 0x0+i,
            'blocks' : 1,
            'core' : (0 + i) % 4,
            'flags' : i+0x100,  
            #'threads' : (i % config_file._threads) + 1,
            'threads' : 1,
            'ramp_msecs' : 0 }
        #print rec_dict
    for i in range(0, 819):
        rec_dict = {
             'opcode' : 1,
            'lba' : 0x50+i,
            'blocks' : 10,
            'core' : (0 + i) % 4,
            'flags' : i+0x100,  
            #'threads' : (i % config_file._threads) + 1,
            'threads' : config_file._threads,
            'ramp_msecs' : 0 }
        #print rec_dict
        rec_list.append(rdgen_record(None, rec_dict))

    for i in range(0, 819):
        rec_dict = {
             'opcode' : 2,
            'lba' : 0x500+i,
            'blocks' : 20,
            'core' : (0 + i) % 4,
            'flags' : i+0x100,  
            #'threads' : (i % config_file._threads) + 1,
            'threads' : 1,
            'ramp_msecs' : 0 }
        #print rec_dict
        rec_list.append(rdgen_record(None, rec_dict))

    filter_dict = {}
    threads = 1
    for tag in config_file._mapping_dict.keys():
        if config_file._interface_dict[tag] == "packet":
            filter_dict[tag] = rdgen_object_filter(config_file._mapping_dict[tag], package_id, threads)
        else:
            filter_dict[tag] = rdgen_device_filter(config_file._mapping_dict[tag], int(tag), threads, config_file._interface_dict[tag])

        write_rdgen_object_file(config_file, filter_dict[tag], rec_list, app.output_file_name)
        threads = threads + 5
    
    write_rdgen_root_file(config_file, app.output_file_name, filter_dict)

