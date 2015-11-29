#!/usr/bin/python

import argparse
from argparse import ArgumentParser
import copy
import time
import re
from os import path
import sys
import collections
from ioprofile import FBE_lun_benchmarks
from utils import TestError, list_unique, is_in_windows
from operator import itemgetter, attrgetter



FDB_version = "0.1.0"
FDB_default_test_time = 140     # 2+ minutes per test.
FDB_run_on_sim = False        # Should run on simulator
FDB_description = "Fbe Rebuild IO benchmark. Version: %s" % FDB_version
FDB_output_verbose = False
FDB_disable_bgop = False
FDB_filename_div_char = '@'
FDB_filename_list_div_char = '+'
FDB_collect_rba = True
FDB_default_count = 1
FDB_default_thread = ["0", "1", "5", "10", "16", "18", "20", "22", "24", "32", "34", "36", "40", "48", "54", "64"]
test_examples = """
============================================================
Examples:
============================================================
To run rebuild benchmark against all bound RAID groups, running default I/O patterns to first LUN in each RG, 
just execute the script with no arguments.
%(prog)s 

To run test 5 (only thread counts 0 and 1) against just RAID group number 10, running I/O to first LUN in RG
%(prog)s -thread_num 0 1 -test 5 -rg_num 10

To run test 3,4 and 5 against just RAID group object ID 0x355 and LUN 0x200, 
        running threads counts of 0, 1 and 2 to first LUN in each RG
%(prog)s -thread_num 0 1 2 -test 3,4,5 -rg 0x355 -lun 0x200
============================================================

"""

if is_in_windows():
    FDB_base_directory = "c:/DUMPS/fbe_rebuild_io_benchmark/"
else:
    FDB_base_directory = "/EMC/C4Core/log/"


FDB_arguments = (
    {
        # version
        "name": ('-version', '-v'),
        "args": {
            "action": "version",
            "version": FDB_description,
            "help": "Show fbe_drive_benchmark version",
        },
    },
    {
        # verbose
        "name": ('-verbose',),
        "args": {
            "help": "Show fbecli output",
            "action": "store_true",
            "default": FDB_output_verbose,
        },
    },
    {
        # Raid group object-id
        "name": ('-rg',),
        "args": {
            "help": "Raid Group object to test. Use format: 0x0010",
        },
    },
    {
        # Number of LUNs per RG to use
        "name": ('-num_luns',),
        "args": {
            "help": "Number of LUNs per raid group to use. (Only used with -rg) Default: " + str(1),
            "type": int,
            "default": 1,
        },
    },
    {
        # RAID Group number list
        "name": ('-rg_num',),
        "args": {
            "help": "RAID Group Number list. Separate RG Numbers with , For example: -rg_num 5,3",
        },
    },
    {
        # LUN object-id
        "name": ('-lun',),
        "args": {
            "help": "LUN num to run IO. Use hex format.  This argument is only used in conjunction with -rg and each LUN needs to be in the same position as the corresponding RAID Group from -rg",
        },
    },
    {
        # tests
        "name": ('-test',),
        "args": {
            "help": "tests to run, by default run all tests",
        },
    },
    {
        # test run time
        "name": ('-time',),
        "args": {
            "help": "Time to run the test (in seconds). Default: " + str(FDB_default_test_time),
            "type": int,
            "default": FDB_default_test_time,
        },
    },
    {
        # Number of 10 second cycles to run I/O
        "name": ('-cycles',),
        "args": {
            "help": "Number of 10 second cycles to run IO. Default: " + str(4),
            "type": int,
            "default": 4,
        },
    },
    {
        # Delay to let rebuild and run I/O in each cycle.
        "name": ('-sleep_time',),
        "args": {
            "help": "seconds delay while running I/O. default is  " + str(10),
            "type": int,
            "default": 10,
        },
    },
    {
        # Percentage we will let rebuild get to before restarting
        "name": ('-max_rb_percent',),
        "args": {
            "help": "Percentage we will let rebuild get to before restarting.  Default is 15%% ",
            "type": int,
            "default": 15,
        },
    },
    {
        # maximum number of iterations per IO profile
        "name": ('-count',),
        "args": {
            "help": "Maximum number of iterations per IO profile. Default: " + str(FDB_default_count),
            "type": int,
            "default": FDB_default_count,
        },
    },
    {
        # set maximum number of threads in any/all IO profile(s)
        "name": ('-thread_num', '-t'),
        "args": {
            "help": "Maximum number of threads in any/all IO profile(s). Default: " + str(FDB_default_thread),
            "nargs": "*",
            "default": FDB_default_thread,
        },
    },
    {
        # Background operation
        "name": ('-bgod',),
        "args": {
            "help": "Disable background service before running test. Default: " + str(FDB_disable_bgop),
            "type": bool,
            "default": FDB_disable_bgop,
        },
    },
    {
        # Collect RBA
        # Background operation
        "name": ('-rba',),
        "args": {
            "help": "Collect RBA during the test. Default: " + str(FDB_collect_rba),
            "action": "store_true",
            "default": FDB_collect_rba,
        },
    },
    {
        # sim
        "name": ("-sim",),
        "args": {
            "help": "Run in simulator. Default: " + str(FDB_run_on_sim),
            "action": "store_true",
            "default": FDB_run_on_sim,
        },
    },
)


class rb_test_config:
    def gen_test_io(self):
        ios = []
        manip_ios = []
        io_hash = collections.OrderedDict()
        #If the IO is a pattern; then populate it from the file
        benchmarks = sorted(FBE_lun_benchmarks, key=attrgetter('number'))
        print "\n"+"="*80
        if self.test:
            for benchmark in benchmarks:
                if str(benchmark.number) in self.test:
                    ios.append(benchmark)
                    print "Running test %d) %s" % (benchmark.number, benchmark.base_test)
        else:
            for benchmark in benchmarks:
                ios.append(benchmark)
                print "Running test %d) %s" % (benchmark.number, benchmark.base_test)
        thread_string = ""
        for thread in self.thread_num:
            thread_string = thread_string + str(thread) + " "
        print "Running threads: " + thread_string
        print "="*80
        ios = sorted(ios, key=attrgetter('number'))  
        #Make list unique again
        ios = list_unique(ios)
        
        #If there are thread numbers expected; populate them
        for io in ios:
            io_pattern_list = []
            if re.search(r"[r|w] \d+ %\(thread\)s \d+", io.cmd_line) is not None:
                if len(self.thread_num) == 0:
                    print "WARNING: Number of threads weren't specified. Thread Pattern RDGEN will not be run\n"
                else:    
                    #print self.thread_num
                    for num in self.thread_num:
                        num = num.strip()
                        io_copy = copy.deepcopy(io)
                        io_copy.multiplier = int(num, 10)
                        io_copy.name = io_copy.name % {'thread': num}
                        if num == "0":
                            io_copy.cmd_line = ""
                        else:
                            io_copy.cmd_line = io_copy.cmd_line % {'thread': ("%x" % io_copy.multiplier)}
                        io_copy.desc = io_copy.desc % {'thread': num}
                        manip_ios.append(io_copy)
            else:
                manip_ios.append(io)
        self.io = list_unique(manip_ios)
        if not self.io:
            raise TestError("No IO Profile specified")
            

    def validate(self):

        for num in self.thread_num:
            if not re.findall("^[0-9]+$", num) or int(num) > 128:
                raise TestError("Thread number %s is not valid, must be decimal and less than 128" % num)
        self.gen_test_io()

    def get_from_file_name(self):
        io_list = []
        if not sys.argv[0]:
            return (io_list)
        name = path.splitext(path.basename(sys.argv[0]))[0]
        args  = name.split(FDB_filename_div_char)
        if len(args) >= 5:
            io_list = args[4].split(FDB_filename_list_div_char)
        return io_list


    def gen_io_name(self, ios):
        names = []
        for io in ios:
            desc = io.get_desc()
            desc = desc % {'thread': "N"}
            names.append("%d) %s\n" % (io.number, desc))
        string = '='*80 + "\nSupported IO Profiles:\n" + '='*80 + "\n" + "".join(names) + '='*80 

        threads = '\n\n' + "Default Threads:\n" + '='*80 + "\n"
        for thread in FDB_default_thread:
            threads = threads + "%s " % thread
        threads = threads + "\n" + '='*80 
        return string + threads
    

    def __init__(self, filename):
        global test_examples
        examples = test_examples % ({'prog' : filename})
        ios = sorted(FBE_lun_benchmarks, key=attrgetter('number'))
        parser = ArgumentParser(description=FDB_description,
                                formatter_class=argparse.RawDescriptionHelpFormatter,
                                usage="%(prog)s -rg 0x0010 -lun 0x0010 -io <benchmark name or all> -iof <iofilename> -t <thread numberss> -time 60 -count 10",
                                epilog=examples + self.gen_io_name(ios))

        for arg in FDB_arguments:
            parser.add_argument(*arg["name"], **arg["args"])

        args = parser.parse_args()
        
        self.desc = FDB_description
        self.version = FDB_version
        self.base_directory = FDB_base_directory
        self.sim = args.sim
        self.time = args.time
        self.rg_obj_id = None
        self.lun_obj_id = None
        self.count = args.count
        self.cycles = args.cycles
        self.thread_num = args.thread_num
        self.disable_bgo = args.bgod
        self.collect_rba = args.rba
        self.verbose = args.verbose
        self.rg_list = None
        self.rg_num_list = None

        if args.rg_num:
            self.rg_num_list = args.rg_num.split(",")
            for rg in self.rg_num_list:
                if not re.findall('^[0-9]+$', rg):
                    raise TestError("unexpected RG num: %s Each RG number should be decimal" % (rg))
        else:
            if args.rg:
                self.rg_list = args.rg.split(",")
                print self.rg_list
                for rg in self.rg_list:
                    if '0x' not in rg:
                        raise TestError("unexpected RG object id: %s each RG Object ID should be hex with 0x" % (rg))
                if args.lun:
                    self.lun_list = args.lun.split(",")
                    print self.lun_list
                    for lun in self.lun_list:
                        if '0x' not in lun:
                            raise TestError("unexpected LUN object id: %s each LUN Object ID should be hex with 0x" % (lun))
                else:
                    raise TestError("Must provide -lun with -rg")
            
        self.num_luns = args.num_luns
        if args.test:
            self.test = args.test.split(",")
        else:
            self.test = None
        self.sleep_time = args.sleep_time
        self.max_rb_percent = args.max_rb_percent
        if self.count <= 0:
            raise ValueError("Invalid count: %d, should > 0" % self.count)
        self.validate()


    def __str__(self):
        desc = self.desc + "\n" + \
               "Version:         " + self.version + "\n" + \
               "Per Test Time:   " + str(self.time) + " seconds\n" + \
               "Per Test Count:  " + str(self.count) + "\n" + \
               "RG To Test:      " + str(self.rg) + "\n" + \
               "LUN To Run IO:   " + str(self.lun) + "\n" + \
               "IO Profile:      " + str(self.io) + "\n" + \
               "Collect RBA:     " + str(self.collect_rba) + "\n" + \
               "Disable BGO:     " + str(self.disable_bgo) + "\n"

        if self.sim:
            desc += "Mode:            simulator\n"
        return desc


if __name__ == "__main__":
    print fdb_config()