#!/usr/bin/python

import time
from subprocess import call
from os import path, makedirs
import os
import re
import platform
import os.path
from prettytable import PrettyTable
from datetime import datetime
import random, string
from collections import defaultdict
import shutil


lun_info_command = """acc -m 1
luninfo -all
"""

rg_info_all_command = """acc -m 1
rginfo -rg all
"""

FBECLI_log_directory = "cli_log"

x = PrettyTable(["Timestamp", "Checkpoints", "mbps"])
x.align["Timestamp"] = "l" # Left align
x.align["Checkpoints"] = "l" # Left align
x.align["mbps"] = "l" # Left align             
x.padding_width = 1

avg = []
globavg = []


def get_path_separator():
    path_separator = '\\'
    if sys.platform == "linux2":
        path_separator = '/'
    return path_separator
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

def get_file(io, value, ext):

    directory = os.getcwd()#get_test_subdir(io, value)
    if not os.path.isdir(directory):
        os.makedirs(directory)

    io_name = io.get_name()
    if io_name:
        io_name = io_name[:10]
    else:
        io_name = "none"
    file = path.join(directory, io_name + "_" + str(value) + ext)

    if(os.path.isfile(file) and io_name != "result"):
        tm = time.localtime()
        rm = path.join(random.choice(string.lowercase) for i in range(10))
        file = path.join(directory, io_name + "_" + value + time_to_filename(tm) + rm + ext)
    return file

def start_neit():
    if is_in_windows():
        start_neit_command = "net start newneitpackage"
        run_command(start_neit_command)

def stop_rba(cfg, rbafile):
    print "\nStop RBA"
    stop_rba_command = "rba.exe -c -t off"
    if cfg.sim and rbafile != "":
        with open(rbafile, "a") as f:
            f.write(stop_rba_command + "\n")
    else:
        run_command(stop_rba_command)

def get_test_subdir(io, value):
    path = os.getcwd() + '\\' + io.base_test + "_" + str(value)
    return path

def start_rba(cfg, io, value):

    rbafile = get_file(io, str(value), ".ktr")
    start_cmd = "rba.exe -o %s -t pdo -t fbe_lun" % rbafile
    print "\nStart RBA: %s" % start_cmd
    #priority_cmd = "rba.exe -j 15"
    if cfg.sim:
        with open(rbafile[0], "a") as f:
            f.write(start_cmd + "\n")
            #f.write(priority_cmd + "\n")
    else:
        run_command(start_cmd)
        #run_command(priority_cmd)
    return rbafile

def move_rba_files(io, base_test, cycle):
    dest_directory = get_test_subdir(io, cycle+1)
    log_files = os.listdir(os.getcwd())
    if not os.path.isdir(dest_directory):
        os.makedirs(dest_directory)
    print "Move files for test %s(%s) -> %s" % (io.base_test, base_test, dest_directory)
    for log_file in log_files:
        if log_file.endswith(".ktr"):
            source = path.join(os.getcwd(), log_file)
            dest = path.join(dest_directory, log_file)
            print "move %s -> %s" % (source, dest)
            shutil.move(source, dest)        
        
def get_avg_rb_rate(checkpoint_progress):
    prevtime = 0
    sum = 0
    count = 0
    for time in checkpoint_progress:
        if prevtime == 0:
            sum = 0
            count = 0
            #filestring = str(time) + "\t" + str(checkpoint_progress[time])
        else:
            deltatime = time - prevtime
            deltachkptmb = (int(checkpoint_progress[time], 0) - int(checkpoint_progress[prevtime], 0))*520/1048576
            mbps = deltachkptmb/deltatime.total_seconds()
            sum = sum + mbps
            count = count + 1
            #filestring = str(time) + "\t" + str(checkpoint_progress[time]) + "\t" + str(deltatime.total_seconds()) + "\t" + str(deltachkpt)
        prevtime = time   
    if count == 0:
        return 0
    else:
        return (sum/count)            
def print_chkpt_progress(rg, io, value, reset):
    global x
    global avg
    checkpoint_progress = rg.checkpoint_progress
        
    if reset:
        x = PrettyTable(["Timestamp", "Checkpoints", "mbps"])
        x.align["Timestamp"] = "l" # Left align
        x.align["Checkpoints"] = "l" # Left align
        x.align["mbps"] = "l" # Left align             
        x.padding_width = 1
        avg = []

    cptfile = get_file(io.get_name() + rg.get_desc(), str(value), ".cpt")
    prevtime = 0
    sum = 0
    count = 0
    for time in checkpoint_progress:
        if prevtime == 0:
            sum = 0
            count = 0
            #filestring = str(time) + "\t" + str(checkpoint_progress[time])
            x.add_row([time, checkpoint_progress[time], "-"])
        else:
            deltatime = time - prevtime
            deltachkptmb = (int(checkpoint_progress[time], 0) - int(checkpoint_progress[prevtime], 0))*520/1048576
            mbps = deltachkptmb/deltatime.total_seconds()
            sum = sum + mbps
            count = count + 1
            x.add_row([time, checkpoint_progress[time], mbps])
            #filestring = str(time) + "\t" + str(checkpoint_progress[time]) + "\t" + str(deltatime.total_seconds()) + "\t" + str(deltachkpt)
        prevtime = time    

    
    if count != 0:
        rb_rate = (sum/count)
        avg.append(rb_rate)
        globavg.append(rb_rate)
    else:
        avg.append(-1)
        globavg.append(-1)
    with open(cptfile, "w") as f:
        #f.write(filestring)
        f.write(x.get_string())
        f.write("\n\n")
        f.write("Average of each iteration:\n")
        for a in avg:
            f.write("%s\n" % str(a))
            
            
def print_avg_result(io):
    global globavg
    rstfile = get_file("result", "all", ".rst")
    threadhex = re.match(r'(.+)_LUN', io.get_name()).group(1).strip()
    with open(rstfile, "a") as rf:
        for a in globavg:
            rf.write("%d\t%s\n" % (int(threadhex, 16), str(a)))
    globavg = []
    
    
def list_unique(l):
    seen = set()
    seen_add = seen.add
    unique = [x for x in l if x not in seen and not seen_add(x)]
    
    objnames = []
    for obj in unique:
        objnames.append(obj.name)
        
    D = defaultdict(list)
    for i, name in enumerate(objnames):
        D[name].append(i)
    D = {k:v for k,v in D.iteritems() if len(v)>1}
    for key in D:
        for val in D[key]:
            rm = ''.join(random.choice(string.lowercase) for i in range(10))
            unique[val].name = key + "_" + rm
    return unique
        

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


                

def parse_lun_info(f):
    found = False
    lun_info = "Logical Unit:\s+(\d+)$"
    lun_object = "Lun Object-id:.+(0x[0-9a-fA-F]+)$"
    rgl = f.readline()
    luns = []
    lun = None
    while rgl:
        #print rgl
        c = re.findall(lun_info, rgl)
        if c:
            lun = int(c[0])
            #print "found lun %s" % (lun)
        d = re.findall(lun_object, rgl)
        if d:
            object_id = int(d[0], 16)
            #print "LUN 0x%x (%d) %u" % (object_id, object_id, lun)
            luns.append({'object_id': object_id, 'lun' : lun})
            lun = None
        rgl = f.readline()
    return luns

def get_lun_objects(cfg):
    #Populate the raid group parameters
    global lun_info_command
    in_file, out_file = get_script_filename("luns.txt")
    run_fbecli(cfg, lun_info_command, in_file, out_file)
    with open(out_file, "r") as rgf:
      return parse_lun_info(rgf)


def parse_rg_info(f):
    found = False
    rg_info = "RAID Group number:\s+(-?\d+) .+object_id: (\d+)"
    luns_list = 'Lun\(s\) in this RG\:(( \d+)+)'
    raid_type = 'Raid Type:\s+(.+)$'
    rgl = f.readline()
    info = {}
    rgs = []
    while rgl:
        c = re.findall(rg_info, rgl)
        if c:
            info['rg_num'] = int(c[0][0])
            info['object_id'] = int(c[0][1])

        a = re.findall(raid_type, rgl)
        if a:
            info['raid_type'] = a[0]

        d = re.findall(luns_list, rgl)
        if d:
            #print rgl
            luns_array = d[0][0].split()
            info['luns'] = luns_array   
            if info['rg_num'] >= 0:
                rgs.append(info)
            #print "RG 0x%x (%d) %u %s" % (info['object_id'], info['object_id'], info['rg_num'], info['raid_type'])
            info = {}
        rgl = f.readline()
    return rgs

def get_rg_objects(cfg):
    #Populate the raid group parameters
    global rg_info_all_command
    in_file, out_file = get_script_filename("rg.txt")
    run_fbecli(cfg, rg_info_all_command, in_file, out_file)
    with open(out_file, "r") as rgf:
      return parse_rg_info(rgf)

def get_rg_info(rg_list, rg_num):
    for rg_info in rg_list:
        if rg_info['rg_num'] == rg_num:
            return rg_info
    return None


def get_lun_info(lun_list, lun):
    for lun_info in lun_list:
        if lun_info['lun'] == lun:
            return lun_info
    return None

def get_rg(rg_list, object_id):
    for rg in rg_list:
        if rg.object_id == object_id:
            return rg
    return None

def parse_specific_rg(rgf, timestamp, rg_list):
   info = {}

   info["object_id"] = ""
   info["rebuild_checkpoint"] = ""
   info["pos"] = ""
   info["lun_list"] = []
   info["disks"] = []
   info["capacity"] = ""
   info["width"] = ""
   info["raid_type"] = ""
   
   rg_info = "RAID Group number:\s+(-?\d+) .+object_id: (\d+)"
   rg_beslist = 'Drives associated with the raid group:(( \d+_\d+_\d+)+)'
   rg_checkpoint = 'Rebuild checkpoint\[\d+\]\.: (0x[0-9a-fA-F]+)'
   luns_list = 'Lun\(s\) in this RG\:(( \d+)+)'
   raid_type = 'Raid Type:\s+(.+)$'
   capacity = 'user Capacity:.+(0x[0-9a-fA-F]+)$'
   width = 'Number of drives in the raid group:\s+(.+)$'
   end_entry = 'Metadata element state'

   found = False
   bes_arr = []
   chk_arr = []
   found = False
   rgl = rgf.readline()
   while rgl:
       #print rgl

       obj = re.findall(rg_info, rgl)
       if obj:
           # Filter out system raid groups.
           if int(obj[0][1]) >= 0x100:
               found = True
               info['rg_num'] = int(obj[0][0])
               info['object_id'] = int(obj[0][1])
           
       if found:
           c = re.findall(luns_list, rgl)
           if c:
               # Luns list is last.
               luns_array = c[0][0].split()
               info["lun_list"] = luns_array

               info["checkpoint_time"] = timestamp
               rg = get_rg(rg_list, info['object_id'])
               if rg == None:
                   found = False
                   info = {"rebuild_checkpoint" : "", "pos" : ""}
                   bes_arr = []
                   chk_arr = []
                   rgl = rgf.readline()
                   continue
               rg.update(info)

               print "RG object id: 0x%x rg_id: 0x%x type: %s " % (info['rg_num'], info['object_id'], info["raid_type"])
               print "    width: %s cap: %s chkpt: %s (%u %%) pos: %s" % (info["width"], info["capacity"], info["rebuild_checkpoint"], rg.get_rb_percent(), info["pos"])
               print "    luns: %s " % (info["lun_list"])
               print "    disks: %s" % (info["disks"])
               info = {"rebuild_checkpoint" : "", "pos" : ""}
               bes_arr = []
               chk_arr = []
               found = False
               rgl = rgf.readline()
               continue
           f = re.findall(raid_type, rgl)
           if f:
               info["raid_type"] = f[0]
           e = re.findall(width, rgl)
           if e:
               info["width"] = int(e[0])
           d = re.findall(capacity, rgl)
           if d:
               info["capacity"] = int(d[0], 16)
           b = re.findall(rg_beslist, rgl)
           if b:
               bes_arr = b[0][0].split()
               info["disks"] = bes_arr
           a = re.findall(rg_checkpoint, rgl)
           if a:
               chk_arr.append(a[0])
               if (a[0] != "0xffffffffffffffff"):
                   #info["object_id"] = ""
                   info["rebuild_checkpoint"] = a[0]
                   info["pos"] = bes_arr[chk_arr.index(a[0])]
       rgl = rgf.readline()
            
   return info

def update_rgs(cfg, rg_list):
    #Populate the raid group parameters
    global rg_info_command
    rg_in_file, rg_out_file = get_script_filename("rg.txt")
    get_rg_command = rg_info_all_command
    timestamp = datetime.now()
    run_fbecli(cfg, get_rg_command, rg_in_file, rg_out_file)
    
    with open(rg_out_file, "r") as rgf:
        parse_specific_rg(rgf, timestamp, rg_list)
