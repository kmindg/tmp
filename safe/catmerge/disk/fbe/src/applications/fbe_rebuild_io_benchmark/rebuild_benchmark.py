#!/usr/bin/python

from os import path,makedirs
import os
import sys
import traceback
from subprocess import call
import threading
from threading import Thread
from datetime import datetime
import time
import re
import argparse
import collections

from utils import *
from options import *

from fbe_rg import fbe_rg
from fbe_lun import fbe_lun
from prettytable import PrettyTable

#These are the raid types we will try to test.
g_valid_raid_types = ["FBE_RAID_GROUP_TYPE_RAID5", "FBE_RAID_GROUP_TYPE_RAID3", "FBE_RAID_GROUP_TYPE_RAID6","FBE_RAID_GROUP_TYPE_RAID1"]
pvd_list_command = """acc -m 1
pvdinfo -list
pdo
"""
# pvdsrvc -object_id 0x%x -mark_offline set
pvd_fault_command = """acc -m 1
pdo_srvc -object_id 0x%x -drive_fault set
"""
pvd_restore_command = """acc -m 1
pvdsrvc -object_id 0x%x -drive_fault clear
"""
rg_info_command = """acc -m 1
rginfo -object_id 0x%x
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
rdgen_end_command = """sleep %u
rdgen -dp
rdgen -term
rdgen -reset_stats
"""

rdgen_finish_start_command = """
rdgen -reset_stats
rdgen -reset_stats
"""
rdgen_stop_command = """acc -m 1
rdgen -dp
rdgen -term
rdgen -reset_stats
"""
rdgen_display_command = """acc -m 1
rdgen -dp
"""
fbe_total_lun_object = "0xFFFFFF"

def parse_rdgen_resp(f, luns):
    def build_id_hash(luns):
        obj_hash = {}
        for d in luns:
            if obj_hash.get(d.object_id):
                print "Warn: duplicate object id:"
                print "    ", obj_hash.get(d.object_id)
                print "    ", d
            obj_hash[d.object_id] = d
        return obj_hash

    def set_obj_rate(obj_dict, obj_id, rate, ios, threads, errors, resp):
        #print "set rate: %s %8.2f" % (obj_id, rate)
        #print obj_dict
        d = obj_dict.get(obj_id)
        if not d:
            #print "WARN: Can't find %s\n" % obj_id
            return

        if d.has_rate:
            print "WARN: dupe rate: %s\n" % obj_id

        d.has_rate = True
        d.rate = rate
        d.avg_response = resp
        if obj_id != fbe_total_lun_object:
            d.io_count = ios
            d.threads = threads
            d.errors = errors
            a = obj_dict.get(int(fbe_total_lun_object, 16))
            if a:
                a.io_count = a.io_count + ios
                a.threads = a.threads + threads
                a.errors = a.errors + errors

    def set_detailed_obj_rate(obj_dict, obj_id, rd_rate, rd_resp, wr_rate, wr_resp):
        #print "set rate: %s %8.2f" % (obj_id, rate)
        #print obj_dict
        d = obj_dict.get(obj_id)
        if not d:
            #print "WARN: Can't find %s\n" % obj_id
            return

        d.has_rate = True
        d.rate = rate
        if obj_id != fbe_total_lun_object:
            d.read_rate = rd_rate
            d.read_avg_response = rd_resp
            d.write_rate = wr_rate
            d.write_avg_response = wr_resp



    #obj_re = "^object_id:.+name:\s+(.+)$"
    obj_re = "^object_id: (.+) interface.+ Packet threads: (.+) ios: 0x(.+) passes: 0x(.+) errors: (.+)$"
    rate_re = "^rate:\s+(\d+.\d+)\s+avg resp tme:\s+(\d+.\d+)"
    read_rate_re = "^read rate:\s+(\d+.\d+)\s+read avg resp tme:\s+(\d+.\d+).+write rate:\s+(\d+.\d+)\s+write avg resp tme:\s+(\d+.\d+)"
    l = f.readline()
    obj_resps = []
    obj_dict = build_id_hash(luns)

    while l:
        overall_rate = re.findall("cumulative io rate .+:\s+(.+)$", l)
        if overall_rate:
            rate = float(overall_rate[0])
            obj_resps.append((fbe_total_lun_object, rate))
            set_obj_rate(obj_dict, int(fbe_total_lun_object, 16), rate, 0, 0, 0, 0)
            #print "overall rate is %2.2f" % (rate)
        obj = re.findall(obj_re, l)
        if obj:
            obj_id = obj[0]
            #print "found %s" % (obj)
            #print l
            threads = int(obj[0][1])
            ios = int(obj[0][2], 16)
            errors = int(obj[0][4])
            #print "threads: %u ios: %u errors: %u" % (threads, ios, errors)
            #print "%s] " % (s)
            s = f.readline()
            #print "%s] " % (s)
            if not s:
                print "WARN: found EOF when parsing '%s'" % l.strip()
                break
            resp = re.findall(rate_re, s)

            if not resp:
                print "WARN: failed to get resp for '%s'" % s.strip()
            else:
                rate = float(resp[0][0])
                response = float(resp[0][1])
                obj_resps.append((obj_id, rate))
                set_obj_rate(obj_dict, int(obj_id[0]), rate, ios, threads, errors, response)
            s = f.readline()
            read_resp = re.findall(read_rate_re, s) 

            if read_resp:
                rd_rate = float(read_resp[0][0])
                rd_resp = float(read_resp[0][1])
                wr_rate = float(read_resp[0][2])
                wr_resp = float(read_resp[0][3])
                set_detailed_obj_rate(obj_dict, int(obj_id[0]), rd_rate, rd_resp, wr_rate, wr_resp)

        l = f.readline()

    #print("Got %u resps" % len(obj_resps))

def time_string():
    now = datetime.now()
    current_date_time = now.strftime('%Y-%m-%d %H:%M:%S')
    return current_date_time

def format_duration(seconds):
    hours   = int(seconds / 3600)
    mins    = int(seconds % 3600) / 60
    secs    = int(seconds % 60)

    string = "%02d:%02d:%02d" % (hours, mins, secs)

    return string
class rg_lunio:
    def __init__(self, cfg):
        self.cfg = cfg
        self.rg = None
        self.lun = None
        self.io = cfg.io
        self.cur_dir = os.getcwd()
        self.orig_out = sys.stdout
        self.orig_err = sys.stderr
        self.results = []
        self.pvd_fault_list = []
        self.end_time = None
        self.max_rebuild_percent = cfg.max_rb_percent # Max percent we will let the rebuild get to.

    def enable_bgo(self):
        in_file,_ = get_script_filename("bgoe.txt")
        run_fbecli(self.cfg, bgoe_command, in_file)


    def disable_bgo(self):
        in_file,_ = get_script_filename("bgod.txt")
        run_fbecli(self.cfg, bgod_command, in_file)


    def parse_rg(self, rgf):
        info = {}

        info["object_id"] = self.cfg.rg_obj_id
        info["rebuild_checkpoint"] = ""
        info["pos"] = ""
        info["lun_list"] = []
        info["disks"] = []
        info["capacity"] = ""
        info["width"] = ""
        info["raid_type"] = ""

        rg_beslist = 'Drives associated with the raid group:(( \d+_\d+_\d+)+)'
        rg_checkpoint = 'Rebuild checkpoint\[\d+\]\.: (0x[0-9a-fA-F]+)'
        luns_list = 'Lun\(s\) in this RG\:(( \d+)+)'
        raid_type = 'Raid Type:\s+(.+)$'
        capacity = 'user Capacity:.+(0x[0-9a-fA-F]+)$'
        width = 'Number of drives in the raid group:\s+(.+)$'
        bes_arr = []
        chk_arr = []
        found = False
        rgl = rgf.readline()
        while rgl:
            #print rgl
            f = re.findall(raid_type, rgl)
            if f:
                info["raid_type"] = f[0]
            e = re.findall(width, rgl)
            if e:
                info["width"] = int(e[0])
            d = re.findall(capacity, rgl)
            if d:
                info["capacity"] = int(d[0], 16)
            c = re.findall(luns_list, rgl)
            if c:
                luns_array = c[0][0].split()
                info["lun_list"] = luns_array
            b = re.findall(rg_beslist, rgl)
            if b:
                bes_arr = b[0][0].split()
                info["disks"] = bes_arr
            a = re.findall(rg_checkpoint, rgl)
            if a:
                chk_arr.append(a[0])
                if (a[0] != "0xffffffffffffffff"):
                    info["object_id"] = ""
                    info["rebuild_checkpoint"] = a[0]
                    info["pos"] = bes_arr[chk_arr.index(a[0])]
            rgl = rgf.readline()
            
        return info

    def parse_rg_out(self, rgf, timestamp):
        info = self.parse_rg(rgf)
        if info:
            info["checkpoint_time"] = timestamp
        return info


    def update_rg(self, rg):
        #Populate the raid group parameters
        global rg_info_command
        rg_in_file, rg_out_file = get_script_filename("rg.txt")
        get_rg_command = rg_info_command
        if re.search('0x%x', get_rg_command) is not None:
            get_rg_command = get_rg_command % rg.object_id
        timestamp = datetime.now()
        run_fbecli(self.cfg, get_rg_command, rg_in_file, rg_out_file)
        with open(rg_out_file, "r") as rgf:
            info = self.parse_rg_out(rgf, timestamp)
            rg.update(info)


    def parse_pvd_info(self, pvdf):
        found = False
        pvd_info = "(0x\w+).+ (\d+)_(\d+)_(\d+).+FBE"
        pdo_info = "^0x([0-9a-fA-F]+).+\|(\d+)_(\d+)_(\d+).+\|.+\|"
        rgl = pvdf.readline()
        pvds = []
        while rgl:
            #print rgl
            c = re.findall(pvd_info, rgl)
            if c:
                object_id = int(c[0][0], 16)
                bus = int(c[0][1])
                encl = int(c[0][2])
                slot = int(c[0][3])
                #print "PVD 0x%x (%d) %d_%d_%d\n" % (object_id, object_id, bus, encl, slot)
                pvds.append({'object_id': object_id, 'bus' : bus, 'slot': slot, 'encl' : encl})

            d = re.findall(pdo_info, rgl)
            if d:
                object_id = int(d[0][0], 16)
                bus = int(d[0][1])
                encl = int(d[0][2])
                slot = int(d[0][3])
                #print "PDO 0x%x (%d) %d_%d_%d\n" % (object_id, object_id, bus, encl, slot)
                for pvd in pvds:
                    if pvd['bus'] == bus and pvd['encl']== encl and pvd['slot']== slot:
                        pvd['pdo_object_id'] = object_id
                        break
            rgl = pvdf.readline()
        return pvds


    def restore_pvd_object(self, pvd):
        #Populate the raid group parameters
        global pvd_restore_command
        restore_cmd = pvd_restore_command
        pvd_in_file, pvd_out_file = get_script_filename("pvd.txt")
        restore_cmd = restore_cmd % pvd['object_id']
        run_fbecli(self.cfg, restore_cmd, pvd_in_file, pvd_out_file)

    def restore_pvd_faults(self):
        for pvd in self.pvd_fault_list:
            disk_str = "%d_%d_%d" % (pvd['bus'], pvd['encl'], pvd['slot'])
            print "restoring pvd 0x%x (%s)\n" % (pvd['object_id'], disk_str)
            self.restore_pvd_object(pvd)
        self.pvd_fault_list = []

    def fault_pvd_object(self, pvd):
        #Populate the raid group parameters
        global pvd_fault_command
        fault_cmd = pvd_fault_command
        pvd_in_file, pvd_out_file = get_script_filename("pvd.txt")
        fault_cmd = fault_cmd % pvd['pdo_object_id']
        run_fbecli(self.cfg, fault_cmd, pvd_in_file, pvd_out_file)
        self.pvd_fault_list.append(pvd)

    def get_pvd_objects(self):
        #Populate the raid group parameters
        global pvd_list_command
        pvd_in_file, pvd_out_file = get_script_filename("pvd.txt")
        run_fbecli(self.cfg, pvd_list_command, pvd_in_file, pvd_out_file)
        with open(pvd_out_file, "r") as rgf:
            return self.parse_pvd_info(rgf)

    def parse_chkpt_out(self, rgf, rg):
        rg_beslist = 'Drives associated with the raid group:(( \d+_\d+_\d+)+)'
        rg_checkpoint = 'Rebuild checkpoint\[%d\]\.: (0x[0-9a-fA-F]+)'
        bes_arr = []
        index = -1
        chk_arr = []
        obj_entry = 'object_id: %d' % rg.object_id
        end_entry = 'Metadata element state'
        found = False
        rgl = rgf.readline()
        while rgl:
            obj = re.findall(obj_entry, rgl)
            if obj:
                found = True
            if found:
                end = re.findall(end_entry, rgl)
                if end:
                    return ""
                b = re.findall(rg_beslist, rgl)
                if b:
                    bes_arr = b[0][0].split()
                    index = bes_arr.index(rg.pos)
                    rg_checkpoint = rg_checkpoint % index
                    
                a = re.findall(rg_checkpoint, rgl)
                if a and index != -1:
                    return a[0]
            rgl = rgf.readline()
        return ""


    def update_chkpt(self, io, cyclenum, timenum, rg_list):
        #Populate the raid group parameters
        global rg_info_command
        rg_in_file, rg_out_file = get_script_filename("rg_%s_cycle%s_time%s.txt" % (io.get_name(), str(cyclenum), str(timenum)))
        
        timestamp = datetime.now()
        run_fbecli(self.cfg, rg_info_all_command, rg_in_file, rg_out_file)

        for rg in rg_list:
            with open(rg_out_file, "r") as rgf:
            
              rg.rebuild_checkpoint = self.parse_chkpt_out(rgf, rg)
              rg.checkpoint_time = timestamp
              rg.checkpoint_progress[rg.checkpoint_time] = rg.rebuild_checkpoint 


    def parse_lun_out(self):
        info = {}
        if self.cfg.lun_obj_id:
            info["object_id"] = self.cfg.lun_obj_id
        return info


    def get_lun(self):
        #Populate the lun parameters
        for lun in self.parse_lun_out():
            lun.get_desc()
            self.lun = lun


    def init_rdgen(self):
        print "\nInitialize RDGEN"
        cmd = rdgen_init_command
        in_file,_ = get_script_filename("rdgen-init.txt")
        run_fbecli(self.cfg, cmd, in_file)


    def gen_io_command(self, io, cyclenum, lun):
        def add_cmd_group(cmd_list, cmd):
            cmd_list.append(rdgen_start_command)
            cmd_list.extend(cmd)
            cmd_list.append(rdgen_finish_start_command)
            #cmd_list.append(rdgen_end_command % self.cfg.time)

        cmd_list = []
        per_cmd = []

        for i in range(cyclenum):
            rdgen_cmd = io.get_cmd_line(lun)
            if rdgen_cmd:
                per_cmd.append(rdgen_cmd)
   
        if per_cmd:
            add_cmd_group(cmd_list, per_cmd)

        return "\n".join(cmd_list)

    def rdgen_stop_io(self):
        running_threads = 1
        in_file = "rdgen_stop.txt"
        run_fbecli(self.cfg, rdgen_stop_command, in_file);
        out_file = get_output_filename(in_file)
        while running_threads > 0:
            print("running_threads: %d" %running_threads)
            run_fbecli(self.cfg, rdgen_stop_command, in_file);
            with open(out_file, "r") as f:
              running_threads = self.check_for_running_threads(f)
              print running_threads
              if running_threads > 0:
                  print("waiting for threads to stop %d running threads" % running_threads)
                  time.sleep(5)
    def check_for_running_threads(self, f):
        l = f.readline()
        while l:
            threads_array = re.findall("threads\s+: (.+)$", l)
            if threads_array:
                threads = int(threads_array[0])
                print("found %d threads running" %threads)
                return threads
            l = f.readline()
        return 0

    def parse_rdgen_results(self, io, cyclenum, lun_objects):
        in_file,out_file = get_script_filename("rdgen_%s_cycle%s.txt" % (io.get_name(), str(cyclenum)))
        with open(out_file, "r") as f:
            parse_rdgen_resp(f, lun_objects)

    def get_rdgen_all_results(self, io, cyclenum, rg_list):
        print "\nget rdgen stats"
        in_file,out_file = get_script_filename("rdgen_%s_cycle%s_rdgen_stats.txt" % (io.get_name(), str(cyclenum)))
        run_fbecli(self.cfg, rdgen_display_command, in_file, out_file)
        print "done getting rdgen stats"
        for rg in rg_list:
            with open(out_file, "r") as f:
                for lun in rg.lun_object_list:
                    lun.reset()
                parse_rdgen_resp(f, rg.lun_object_list)

    def get_rdgen_results(self, io, cyclenum, lun_objects):
        print "\nget rdgen stats"
        in_file,out_file = get_script_filename("rdgen_%s_cycle%s_rdgen_stats.txt" % (io.get_name(), str(cyclenum)))
        run_fbecli(self.cfg, rdgen_display_command, in_file, out_file)
        print "done getting rdgen stats"

        with open(out_file, "r") as f:
            for lun in lun_objects:
                lun.reset()
            parse_rdgen_resp(f, lun_objects)

    def print_results(self, rg_list):
      results_file = path.join(os.getcwd(), "total_results.txt")
      with open(results_file, "w") as f:
       
        for rg in rg_list:

            lun = rg.lun_object_list[0]
            header = "RAID Group: 0x%x LUN: 0x%x\n" % (rg.object_id, lun.object_id)

            index = 0
            for basis_test in rg.results.keys():
                basis_test_list = rg.results[basis_test]
                table = PrettyTable(["test", "rebuild drive", "mutiplier", "threads", "rebuild_rate", 
                                     "total rate io/s", "read rate", "write rate", "total resp time", "read response", "write response"])
                table.align["Timestamp"] = "l" # Left align
                table.padding_width = 1

                print header
                f.write(header) 
                for t in basis_test_list:
                    for lun in rg.lun_object_list:
                        if lun.object_id != int(fbe_total_lun_object, 16):
                            table.add_row([t['io'], t['rebuild_drive'], t['io'].multiplier, t['threads'], t['rb_rate'], t['rate'], t['read_rate'], t['write_rate'], t['avg_response'], t['read_response'], t['write_response']])
                
                    index = index + 1
                print table.get_string()
                print "\n\n"
                f.write(table.get_string())
                f.write("\n\n")

    def print_csv(self, rg_list):
        in_file = path.join(os.getcwd(), "total_results.csv")
        with open(in_file, "w") as f:
          header = ["test", "mutiplier", "threads", "rebuild_rate", "total rate io/s", "read rate", "write rate", "total resp time", "read response", "write response"]
          header_string = ""
          for item in header:
              header_string = header_string + item + ","
          header_string = header_string + '\n'
          index = 0
          for rg in rg_list:

              lun = rg.lun_object_list[0]
              target_info = "RAID Group: 0x%x LUN: 0x%x\n" % (rg.object_id, lun.object_id)

              for basis_test in rg.results.keys():
                  basis_test_list = rg.results[basis_test]
                  test_string = "test %d) %s %s\n" % (index, basis_test, basis_test_list[0]['io'].desc)
                  f.write(target_info)
                  f.write(test_string)
                  f.write(header_string)

                  for t in basis_test_list:
                      for lun in rg.lun_object_list:
                          if lun.object_id != int(fbe_total_lun_object, 16):
                              lun_stats = lun.get_stats()
                              csv_line = "%s,%u,%u, %f, %f,%f,%f, %f,%f,%f\n" % (t['io'], t['io'].multiplier, t['threads'], t['rb_rate'], t['rate'], t['read_rate'], 
                                  t['write_rate'], t['avg_response'], t['read_response'], t['write_response'])
                              f.write(csv_line)
                

    def start_io(self, io, cyclenum, lun):
        #print "\nRun RDGEN"
        cmd = self.gen_io_command(io, cyclenum, lun)
        in_file,out_file = get_script_filename("rdgen_%s_cycle%s.txt" % (io.get_name(), str(cyclenum)))
        run_fbecli(self.cfg, cmd, in_file, out_file)
        #print "done running rdgen"
    
    def wait_for_rb_restart(self, rg, swap_disk):
        
        self.update_rg(rg)
        while (swap_disk in rg.disks) or not rg.is_rebuild_started():
            print "="*80
            print "Rebuilding has not re-started yet. Sleeping... chkpt: %s percent: %u" % (rg.rebuild_checkpoint, rg.get_rb_percent())
            if rg.swap_disk != "":
                disk_string = ""
                for d in rg.disks:
                    disk_string = disk_string + d + " "
                print "RAID Group 0x%x waiting for disk %s to swap out current disks: %s " % (rg.object_id, swap_disk, disk_string)
                print "="*80
            time.sleep(3)
            self.update_rg(rg)
        disk_string = ""
        for d in rg.disks:
            disk_string = disk_string + d + " "
        print "RAID Group 0x%x Rebuild is running. Chkpt: %s percent: %u disks: %s" % (rg.object_id, rg.rebuild_checkpoint, rg.get_rb_percent(), disk_string)
        if rg.swap_disk != "":
            print "RAID Group 0x%x disk %s swapped out" % (rg.object_id, swap_disk)
            

    def wait_for_rb_start(self, rg):
        count = 0
        while not self.ispass_initcondn(rg):
            print "Rebuilding hasn't started yet. Sleeping... chkpt: %s percent: %u" % (rg.rebuild_checkpoint, rg.get_rb_percent())
            time.sleep(10)
            self.update_rg(rg)
            if rg is None:
                raise TestError("No raid group for the object id: " + rg.object_id)
            count += 1
            if(count > 5):
                raise TestError("Rebuilding hasn't started yet. Tried sleeping for 5 minutes, quiting...")

    def find_pvd_object(self, disk, pvd_list):
        for pvd in pvd_list:
            search_disk = "%d_%d_%d" % (pvd['bus'], pvd['encl'], pvd['slot'])
            if search_disk == disk:
                return pvd
        return None
    def check_for_rebuild(self, rg_list):
        update_rgs(self.cfg, rg_list)
        #for rg in rg_list:
        #    self.update_rg(rg)
        pvd_list = self.get_pvd_objects()

        for rg in rg_list:
            rb_percent = rg.get_rb_percent()
            if rb_percent >= self.max_rebuild_percent:
                print "RAID Group object: 0x%x RB Percentage is %d" % (rg.object_id, rb_percent)
                # Restart rebuild
                if rg.pos != "":
                    # We are already rebuilding, remove the drive we are rebuilding.
                    disk = rg.pos
                else:
                    # Choose a position to degrade.
                    disk = rg.disks[0]
                pvd = self.find_pvd_object(disk, pvd_list)
                if pvd == None:
                    raise TestError("PVD not found for disk: " + disk)
                rg.swap_disk = disk
                print "Fault disk %s (PVD object id: 0x%x)\n" % (disk, pvd['object_id'])
                self.fault_pvd_object(pvd)
        for rg in rg_list:
            if (rg.swap_disk in rg.disks) or not rg.is_rebuild_started():
                self.wait_for_rb_restart(rg, rg.swap_disk)
            rg.swap_disk = ""
    
    def get_rg_list(self):
        global g_valid_raid_types
        lun_list = get_lun_objects(self.cfg)
        rg_info_list = get_rg_objects(self.cfg)
        rg_list = []
        rg_num_list = self.cfg.rg_num_list
        if rg_num_list == None or len(rg_num_list) == 0:
            rg_num_list = []
            print "No RAID Groups provided, will test all user raid groups."
            for rg_info in rg_info_list:
                if rg_info['object_id'] >= 0x100 and rg_info['raid_type'] in g_valid_raid_types:
                    rg_num_list.append(rg_info['rg_num'])
        if len(rg_num_list) == 0:
            raise TestError("Found no RAID Groups to test")
        print "\n" + "="*80
        for rg_num in rg_num_list:
            found = False
            rg_info = get_rg_info(rg_info_list, int(rg_num))
            if rg_info == None:
                raise TestError("Selected RAID Group %s not found. " % rg_num)
            rg = fbe_rg(str(rg_info['object_id']))
            found = True
            index = 0
            for lun in rg_info['luns']:
                if index >= self.cfg.num_luns:
                    break
                lun_info = get_lun_info(lun_list, int(lun))
                if lun_info == None:
                    print "Could not find lun information for lun %u" % int(lun)
                    print lun_list
                    raise TestError("lun not found")
                rg.lun = fbe_lun({"object_id" : str(lun_info['object_id'])})
                rg.lun_object_list.append(rg.lun)
                print "Testing RG (%d) 0x%x (%s) with LUN %u" % (rg_info['rg_num'], rg.object_id, rg_info['raid_type'], lun_info['object_id'])
                index = index + 1

            rg_list.append(rg)

        print "="*80
        return rg_list
            
    def run_test_rg_object_ids(self):
        print zip(self.cfg.rg_list, self.cfg.lun_list)
        rg_list = []

        info = {}
        info["object_id"] = fbe_total_lun_object
        self.lun_list = []
        self.lun_list.append(fbe_lun(info))

        for rg_obj_id,lun_obj_id in zip(self.cfg.rg_list, self.cfg.lun_list):

            rg = fbe_rg(rg_obj_id)
            rg.lun = fbe_lun({"object_id" : lun_obj_id})
            rg_list.append(rg)
            rg.rebuild_checkpoint = "0x800"
            self.lun_list.append(rg.lun)
            rg.lun_object_list.append(rg.lun)

        self.run_rg_test(rg_list)

    def run_test(self):
        #print self.cfg.rg_num_list

        # If the user provided object ids, then use them.
        if self.cfg.rg_list:
            self.run_test_rg_object_ids()
        else:
            # Otherwise use the raid group number list and derive the LUN #s.
            rg_list = self.get_rg_list()
            self.run_rg_test(rg_list)

    def print_test_progress(self, io, index):
        print "\n" + "="*80
        pct = ((index + 1) * 100) / len(self.io)
        if index == 0:
            print "%s start test %s %u of %u " % (time_string(), io.get_name(), index + 1, len(self.io))
        else:
            elapsed_seconds = time.time() - self.start_time
            seconds_per_test = elapsed_seconds / index
            seconds_remaining = (len(self.io) - index) * seconds_per_test
            print "%s start test %s %u of %u (%u %%)" % (time_string(), io.get_name(), index + 1, len(self.io), pct)
            print "elapsed time:        %s" % (format_duration(elapsed_seconds))
            print "average time per test:  %s" % (format_duration(seconds_per_test))
            print "estimated time remaining: %s" % (format_duration(seconds_remaining))
        print "="*80

    def run_rg_test(self, rg_list):
        self.start_time = time.time()
        self.init_rdgen()
        rbafile = ""
        #For different IO Profiles
            #for i in range(self.cfg.count):
        base_test = ""
        index = 0
        for io in self.io:
            start_test_time = time.time()

            self.print_test_progress(io, index)

            if base_test != "" and base_test != io.base_test:
                # Because the path length rba can support is limited, we need to move the files after the test...
                move_rba_files(last_test, base_test, 1)
            self.result = {}
            stop_rba(self.cfg, rbafile)
            
            self.check_for_rebuild(rg_list)
            self.restore_pvd_faults()
            for rg in rg_list:
                if io.multiplier != 0:
                    self.start_io(io, 1, rg.lun)
                rg.checkpoint_progress = collections.OrderedDict()
            time.sleep(5)
            #Check rebuild progress every 10 seconds
            print "\nGet Checkpoint Progress"

            for j in range(0,self.cfg.cycles):
                print "%s cycle %d" % (time_string(), j)
                # After 2 loops start collecting RBAs.
                if(j == 2 and self.cfg.collect_rba):                        
                    rbafile = start_rba(self.cfg, io, 1)
                    print "\nGet Checkpoint Progress"

                self.update_chkpt(io, 1, j+1, rg_list)
                for rg in rg_list:
                    if rg.rebuild_checkpoint == "":
                        raise TestError("No checkpoint progress for bes: " + rg.pos)
                if self.cfg.sleep_time:
                    time.sleep(self.cfg.sleep_time)
            stop_rba(self.cfg, rbafile)
            base_test = io.base_test
            last_test = io
            self.get_rdgen_all_results(io, 1, rg_list)
            for rg in rg_list:
                #self.get_rdgen_results(io, 1, rg.lun_object_list)
                rb_rate = get_avg_rb_rate(rg.checkpoint_progress)
                if io.base_test not in rg.results:
                    rg.results[io.base_test] = []
                    
                result_rg = rg
                result = { "rg" : result_rg,
                           "io": io,
                           "rebuild_drive" : result_rg.pos,
                           "rb_rate": rb_rate,
                           "threads": rg.lun_object_list[0].threads,
                           "rate": rg.lun_object_list[0].rate,
                           "read_rate": rg.lun_object_list[0].read_rate,
                           "write_rate": rg.lun_object_list[0].write_rate,
                           "avg_response": rg.lun_object_list[0].avg_response,
                           "read_response": rg.lun_object_list[0].read_avg_response,
                           "write_response": rg.lun_object_list[0].write_avg_response}
                rg.results[io.base_test].append(result)
                
            self.rdgen_stop_io()
            self.print_results(rg_list)
            self.print_csv(rg_list)

            duration = time.time() - start_test_time
            print "\n" + "="*80
            print "test %s duration was: %s " % (io.get_name(), format_duration(duration))
            print "="*80
            index = index + 1
             
        # Because the path length rba can support is limited, we need to move the files after the test...
        move_rba_files(last_test, base_test, 1)

        self.end_time = time.time()
        test_duration = self.end_time - self.start_time
        print "total test duration was: %s " % (format_duration(test_duration))


    def ispass_initcondn(self, rg):

        if rg.rebuild_checkpoint == "" or rg.pos == "" or rg.rebuild_checkpoint == "0x0":
            return False
        else:
            return True


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

        makedirs(FBECLI_log_directory)
        start_neit()
        if self.cfg.disable_bgo:
            self.disable_bgo()
        self.rdgen_stop_io()


    def stop_rdgen(self):
        cmd = rdgen_end_command
        in_file,_ = get_script_filename("rdgen-end.txt")
        run_fbecli(self.cfg, cmd, in_file)


    def cleanup(self):
        if self.cfg.collect_rba:
            stop_rba(self.cfg, "")

        if self.cfg.disable_bgo:
            self.enable_bgo()

        self.restore_pvd_faults()

        os.chdir(self.cur_dir)
        sys.stdout = self.orig_out
        sys.stderr = self.orig_err


def main():
    cfg = rb_test_config(os.path.basename(sys.path[0]))
    test = rg_lunio(cfg)
    try:
        for i in range(cfg.count):
            print "="*80
            print "%s Starting iteration %u of %u" % (time_string(), i+1, cfg.count)
            print "="*80
            test.setup()
            test.run_test()

    except KeyboardInterrupt:
        print "Got CTRL-C; Shutting down"
        test.cleanup()
    except TestError as e:
        print e
    except Exception as e:
        traceback.print_exc()
        print e
    finally:
        test.cleanup()

if __name__ == "__main__":
    try:
        main()
    except TestError as e:
        print e
        sys.exit(1)
    except Exception as e:
        traceback.print_exc()
        sys.exit(1)

    
