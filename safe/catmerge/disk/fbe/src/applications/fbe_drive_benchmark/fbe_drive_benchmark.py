#!/usr/bin/python

from os import path,makedirs
import os
import sys
import traceback
from subprocess import call
import time
import re
import argparse

from utils import *
from options import *

from fbe_drive import drive_pool, fbe_drive
from viewer import fbe_drive_benchmark_get_viewers
from benchmarks import get_precondition_bm


disk_info_command = """acc -m 1
di
"""
pvd_info_command = """acc -m 1
pvdinfo -list
"""
pvdcap_info_command = """acc -m 1
pvdinfo
"""
bgoe_command = """acc -m 1
bgoe -all
"""
bgod_command = """acc -m 1
bgod -all
"""
rdgen_start_command = """acc -m 1
rdgen -reset_stats
"""
rdgen_init_command = """acc -m 1
rdgen -init_dps 500
rdgen -dp
rdgen -term
rdgen -reset_stats
"""
rdgen_end_command = """rdgen -dp
rdgen -term
sleep %u
rdgen -reset_stats
"""

class drive_benchmark:
    def __init__(self, cfg):
        self.cfg = cfg
        self.test_drive_pool = drive_pool()
        self.all_drive_pool = drive_pool()
        self.benchmarks = cfg.benchmark
        self.cur_dir = os.getcwd()
        self.orig_out = sys.stdout
        self.orig_err = sys.stderr


    def enable_bgo(self):
        in_file,_ = get_script_filename("bgoe.txt")
        run_fbecli(self.cfg, bgoe_command, in_file)


    def disable_bgo(self):
        in_file,_ = get_script_filename("bgod.txt")
        run_fbecli(self.cfg, bgod_command, in_file)


    def parse_di(self, f):
        info = {}
        keys = ((r"Object id:\s*(\w+)", "object_id"),
                (r"Vendor ID:\s*(\w+.*)", "vendor"),
                (r"Product ID:\s*(\w+.*)", "product_id"),
                (r"Product Rev:\s*(\w+.*)", "product_rev"),
                (r"Product Sr\.No\.:\s*(\w+.*)", "sn"),
                (r"Drive Capacity:\s*(\w+.*)", "capacity"),
                (r"Drive Type:\s*(\w+.*)", "drive_type"),
                (r"Drive RPM:\s*(\w+)", "rpm"),
                (r"Drive TLA info:\s*(\w+)", "tla"))

        l = f.readline()
        while l:
            if re.findall("^-+$", l):
                break
            s = l.strip()
            for key in keys:
                re_str = key[0]
                value = re.findall(re_str, s)
                if value:
                    info[key[1]] = value[0].strip()
            l = f.readline()
        return info


    def parse_capacity(self, f):
        pvdinfo = {}
        l = f.readline()
        while l:
            b = re.search('Obj ID | Disk', l)
            if b is not None:
                done = False
                l = f.readline()
                bes = []
                cap = 0
                while l and (not done):
                    c = re.search('\|\d+_\d+_\d+', l)
                    d = re.search('Provision Drive Exported Capacity:', l)
                    if c is not None:
                        e = re.search('READY', l)
                        if e is not None:
                            pvdinfo_row = l.split("|")
                            obj = pvdinfo_row[0].strip()
                            bes = pvdinfo_row[1].strip().split("_")
                            bes = map(int, bes)
                            bes = map(str, bes)
                            key = bes[0] + "_" + bes[1] + "_" + bes[2]
                            pvdinfo.setdefault(key, [])
                            pvdinfo[key].append(obj)
                        else:
                            done = True
                    if d is not None:
                        pvdinfo_row = l.split(":")
                        cap = int(pvdinfo_row[1].strip(), 0)
                        if cap < 0:
                            cap = 0;
                        if key and (cap >= 0):
                            cap = str(cap)
                            pvdinfo[key].append(cap)
                        done = True
                    l = f.readline()
            l = f.readline()
        return pvdinfo


    def parse_pvd(self, f):
        pvdinfo = {}
        for i, l in enumerate(f):
            if i > 1:
                pvdinfo_row = l.split()
                bes = pvdinfo_row[1].strip().split("_")
                bes = map(int, bes)
                bes = map(str, bes)
                if pvdinfo_row:
                    pvdinfo[bes[0] + "_" + bes[1] + "_" + bes[2]] = pvdinfo_row[0].strip()
        return pvdinfo


    def parse_di_pvd_out(self, dif, pvdf, pvdc):
        disk_header = r"^Disk - (\d+)_(\d+)_(\d+)"
        #pvdinfo = self.parse_pvd(pvdf)
        pvdcapacity = self.parse_capacity(pvdc)

        dil = dif.readline()
        while dil:
            a = re.findall(disk_header, dil)
            if a:
                pos = a[0]
                info = self.parse_di(dif)
                if info:
                    bes = pos[0] + '_' + pos[1] + '_' + pos[2]
                    if bes in pvdcapacity:
                        info["pvd_obj_id"] = pvdcapacity[bes][0]
                        info["pvd_capacity"] = pvdcapacity[bes][1]
                    else:
                        info["pvd_obj_id"] = '0x0'
                        info["pvd_capacity"] = '0'
                    info["pos"] = (int(pos[0]), int(pos[1]), int(pos[2]))
                    yield fbe_drive(info)
            dil = dif.readline()


    def get_test_drives(self):
        """Generate drive pool for testing from cmd line options
        """
        test_drives = []
        for pattern in self.cfg.drive_list:
            for drive in self.all_drive_pool.drive_list:
                if drive.is_match(pattern, self.cfg.flash_only):
                    test_drives.append(drive)
        test_drives = list_unique(test_drives)
        for drive in test_drives:
            self.test_drive_pool.add_drive(drive)
        if self.test_drive_pool.is_empty():
            raise TestError("No drives found for " + str(self.cfg.drive_list))


    def build_all_drive_list(self):
        """Find out all drives and build a fbe_drive list
        """
        di_in_file,di_out_file = get_script_filename("di.txt")
        run_fbecli(self.cfg, disk_info_command, di_in_file, di_out_file)
        pvd_in_file,pvd_out_file = get_script_filename("pvdinfo.txt")
        run_fbecli(self.cfg, pvd_info_command, pvd_in_file, pvd_out_file)
        pvdcap_in_file,pvdcap_out_file = get_script_filename("pvdcapinfo.txt")
        run_fbecli(self.cfg, pvdcap_info_command, pvdcap_in_file, pvdcap_out_file)
        with open(di_out_file, "r") as dif, open(pvd_out_file, "r") as pvdf, open(pvdcap_out_file, "r") as pvdc:
            for drive in self.parse_di_pvd_out(dif, pvdf, pvdc):
                drive.get_desc()
                self.all_drive_pool.add_drive(drive)


    def _set_speed(self, bm, object_id, rate):
        disk = None
        if re.findall("PDO", bm.get_name()):
            disk = self.test_drive_pool.find_drive_by_id(object_id)
        elif re.findall("PVD", bm.get_name()):
            disk = self.test_drive_pool.find_drive_by_pvd(object_id)
    
        if not disk:
            print "WARN: can't find drive for object id %s" % hex(object_id)
        else:
            disk.set_speed(bm, rate)


    def init_rdgen(self):
        cmd = rdgen_init_command
        in_file,_ = get_script_filename("rdgen-init.txt")
        run_fbecli(self.cfg, cmd, in_file)


    def parse_rdgen_resp(self, f):
        obj_re = "^object_id:\s+(\d+)"
        rate_re = "^rate:\s+(\d+.\d+)"
        l = f.readline()
        while l:
            obj = re.findall(obj_re, l)
            if obj:
                obj_id = int(obj[0])
                s = f.readline()
                if not s:
                    print "WARN: found EOF when parsing '%s'" % l.strip()
                    break
                resp = re.findall(rate_re, s)
                if resp:
                    rate = float(resp[0])
                    yield (obj_id, rate)
                else:
                    print "WARN: failed to get resp for '%s'" % l.strip()
            l = f.readline()


    def gen_benchmark_command(self, bm):
        def add_cmd_group(cmd_list, cmd):
            cmd_list.append(rdgen_start_command)
            cmd_list.extend(cmd)
            cmd_list.append(rdgen_end_command % self.cfg.time)

        disk_list = self.test_drive_pool.drive_list
        cmd_list = []
        per_cmd = []
        count = 0
        for disk in disk_list:
            rdgen_cmd = bm.get_cmd_line(disk)
            if rdgen_cmd:
                per_cmd.append(rdgen_cmd)
                count += 1
                if count == self.cfg.max_drives:
                    add_cmd_group(cmd_list, per_cmd)
                    count = 0
                    per_cmd = []
        if per_cmd:
            add_cmd_group(cmd_list, per_cmd)

        return "\n".join(cmd_list)


    def run_benchmark(self, bm):
        cmd = self.gen_benchmark_command(bm)
        in_file,out_file = get_script_filename("rdgen-%s.txt" % bm.get_name())
        run_fbecli(self.cfg, cmd, in_file, out_file)
        with open(out_file, "r") as f:
            for object_id, rate in self.parse_rdgen_resp(f):
                self._set_speed(bm, object_id, rate)


    def save_result(self):
        test_result =  {
            "config": self.cfg,
            "test_time": (self.start_time, self.end_time),
            "test_drives": self.test_drive_pool,
            "benchmarks": self.benchmarks,
        }
        result_viewers = fbe_drive_benchmark_get_viewers()
        for viewer in result_viewers:
            v = viewer(test_result)
            v.log()

    def  precondition_drives(self):
        bm = get_precondition_bm()
        self.run_benchmark(bm)

    def run_test(self):
        self.start_time = time.localtime()

        self.build_all_drive_list()
        self.get_test_drives()
        self.init_rdgen()

        for bm in self.benchmarks:
            print "\n--------------------------------------------------------\n"
            if self.cfg.collect_rba:
                stop_rba(cfg, bm)

            if self.cfg.flash_only:
                self.precondition_drives()

            if self.cfg.collect_rba:
                start_rba(cfg, bm)
                
            self.run_benchmark(bm)
        self.end_time = time.localtime()


    def setup(self):
        cur_time = time.localtime()

        work_dir = get_work_dir(self.cfg, cur_time)
        makedirs(work_dir)
        os.chdir(work_dir)

        print "Test result will be saved in", os.getcwd()
        # Redirect stdout to log.txt
        f = open("log.txt", "w")
        tee = tee_writer(f, sys.stdout)
        sys.stdout = tee
        sys.stderr = tee

        print self.cfg

        makedirs(FBECLI_log_directory)
        start_neit()
        if self.cfg.disable_bgo:
            self.disable_bgo()


    def cleanup(self):
        for bm in self.benchmarks:
            if self.cfg.collect_rba:
                stop_rba(cfg, bm)

        if self.cfg.disable_bgo:
            self.enable_bgo()

        os.chdir(self.cur_dir)
        sys.stdout = self.orig_out
        sys.stderr = self.orig_err


def main():
    try:
        test = drive_benchmark(cfg)
        test.setup()
        test.run_test()
        test.save_result()
    except TestError as e:
        print e
    except Exception as e:
        traceback.print_exc()
        print e
    finally:
        test.cleanup()

if __name__ == "__main__":
    try:
        cfg = fdb_config()
    except TestError as e:
        print e
        sys.exit(1)
    except Exception as e:
        traceback.print_exc()
        sys.exit(1)

    main()
