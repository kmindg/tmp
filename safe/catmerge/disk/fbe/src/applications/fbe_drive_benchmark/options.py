#!/usr/bin/python

import argparse
from argparse import ArgumentParser
import time
import re
from os import path
import sys
from benchmarks import fbe_drive_find_benchmark, FBE_drive_benchmarks
from utils import TestError, list_unique, is_in_windows

FDB_version = "0.1.0"

FDB_default_test_time = 60     # 1 minute per test.
FDB_run_on_sim = False        # Should run on simulator
FDB_description = "Fbe driver benchmark. Version: %s" % FDB_version
FDB_default_count = 10
FDB_output_verbose = False
FDB_disable_bgop = True
FDB_filename_div_char = '@'
FDB_filename_list_div_char = '+'
FDB_collect_rba = False
FDB_flash = False

if is_in_windows():
    FDB_base_directory = "c:/DUMPS/fbe_drive_benchmark/"
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
        # Drive list
        "name": ("drives",),
        "args": {
            "help": "Drive to test. Use format: all 1_1_0 1_2_* ST0VMSIM*",
            "nargs": "*",
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
        # maximum drives per test
        "name": ('-count',),
        "args": {
            "help": "Maximum drive number per test. Default: " + str(FDB_default_count),
            "type": int,
            "default": FDB_default_count,
        },
    },
    {
        # Benchmark
        "name": ('-benchmark', "-bm"),
        "args": {
            "help": "Specific benchmarks. 'all' for all benchmarks",
            "nargs": "*",
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
    {
        # run flash tests
        "name": ("-flash",),
        "args": {
            "help": "Flash drive tests only. Default: " + str(FDB_flash),
            "action": "store_true",
            "default": FDB_flash,
        },
    },
)

class fdb_config:
    def gen_test_benchmarks(self):
        benchmarks = []
        for name in self.benchmark:
            bm = fbe_drive_find_benchmark(name)
            if not bm:
                raise TestError("No benchmark for %s" % name)
            benchmarks.extend(bm)
        self.benchmark = list_unique(benchmarks)
        if not self.benchmark:
            raise TestError("No benchmark specified")


    def validate(self):
        if not self.drive_list:
            raise TestError("No drives to test")
        if not self.benchmark:
            raise TestError("No benchmarks to test")
        else:
            self.gen_test_benchmarks()


    def get_from_file_name(self):
        drive_list = []
        bm_list = []
        if not sys.argv[0]:
            return (drive_list, bm_list)
        name = path.splitext(path.basename(sys.argv[0]))[0]
        args  = name.split(FDB_filename_div_char)
        if len(args) >= 2:
            drive_list = args[1].split(FDB_filename_list_div_char)
            if len(args) >= 3:
                bm_list = args[2].split(FDB_filename_list_div_char)
        return (drive_list, bm_list)


    def gen_benchmark_name(self):
        names = []
        for bm in FBE_drive_benchmarks:
            desc = bm.get_desc()
            names.append("    %s" % desc)
        return "\n".join(names)

    def __init__(self):
        parser = ArgumentParser(description=FDB_description,
                                formatter_class=argparse.RawDescriptionHelpFormatter,
                                usage="%(prog)s 0_0_1 0_1* -benchmark b0 b1 -time 60 -count 5",
                                epilog="Supported Benchmraks:\n" + self.gen_benchmark_name())

        for arg in FDB_arguments:
            parser.add_argument(*arg["name"], **arg["args"])

        args = parser.parse_args()

        self.desc = FDB_description
        self.version = FDB_version
        self.base_directory = FDB_base_directory
        self.sim = args.sim
        self.time = args.time
        self.max_drives = args.count
        self.drive_list = args.drives
        self.benchmark = args.benchmark
        self.disable_bgo = args.bgod
        self.collect_rba = args.rba
        self.verbose = args.verbose
        self.flash_only = args.flash

        if self.max_drives <= 0:
            raise ValueError("Invalid count: %d, should > 0" % self.max_drives)

        def_drive_list, def_bm_list = self.get_from_file_name()
        if not self.drive_list:
            self.drive_list = def_drive_list
        if not self.benchmark:
            self.benchmark = def_bm_list
        self.validate()


    def __str__(self):
        desc = self.desc + "\n" + \
               "Version:         " + self.version + "\n" + \
               "Per Test Time:   " + str(self.time) + " seconds\n" + \
               "Per Test Count:  " + str(self.max_drives) + "\n" + \
               "Drive To Test:   " + str(self.drive_list) + "\n" + \
               "Benchmarks:      " + str(self.benchmark) + "\n" + \
               "Collect RBA:     " + str(self.collect_rba) + "\n" + \
               "Disable BGO:     " + str(self.disable_bgo) + "\n"

        if self.sim:
            desc += "Mode:            simulator\n"
        return desc


if __name__ == "__main__":
    print fdb_config()
