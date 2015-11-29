#!/usr/bin/python

import sys
import os
import time
from struct import *
from rbadef import *
from rbarecord import make_record, filter_record, record_list

class rba_file:
    def __init__(self, file_name = ""):
        self._file_name = file_name
        self.info = {}
        self.records = record_list()
        self.rtc_usec = 1.0
        self.boot_time = 0
        self.boot_time_stamp = 0

    def print_header(self, header):
        for key in rba_header_desc:
            v = header[key]
            if (type(v) == int or type(v) == long):
                print "%s: %u 0x%x" % (key, v, v)
            else:
                print "%s: %s" % (key, v)

    def get_record_desc(self, record):
        traffic_name = get_rba_traffic_id_name(record.tid)
        obj_name = record.get_id_name()
        cmd_name = get_rba_traffic_cmd_name(record.cmd)
        is_finished = record.rba_end != 0
        start_time = stamp_to_utc(record._ts,
                                  self.boot_time, self.boot_time_stamp,
                                  self.rtc_usec)

        time_info = get_rba_time_desc(start_time)
        cpu_info = "%2u" % record.cpu
        obj_info = "%5s-%-10s" % (traffic_name, obj_name)
        block_info = "LBA 0x%016x, BLKS 0x%-8x  Q %-4u" \
                     % (record.lba, record.count, record.queue_io)
        info = time_info + " " + cpu_info + " " + obj_info + " " + block_info
        info += "%-5s" % cmd_name
        info += " %-4x" % record.get_raw_cmd() #record.cmd
        if is_finished:
            rsp_time = stamp_to_ms(record.rt, self.rtc_usec)
            info += " Resp %.3fms" % rsp_time
        else:
            info += " Pending"
        if record.priority < priority_dict['LOW'] or record.priority > priority_dict['URGENT']:
            info += " pri-%d" % record.priority
        else:
            info += " pri-%s" % get_priority_key_by_value(record.priority)
        return info

    def calc_record_time(self, rl):
        for r in rl:
            start_xsec = stamp_to_abs_xsec(r._ts, self.boot_time,
                                           self.boot_time_stamp, self.rtc_usec)
            rsp_time = stamp_to_ms(r.rt, self.rtc_usec)
            r.start_xsec = start_xsec
            r.resp = rsp_time

    def read_header(self, f):
        header_data = f.read(512)
        self.info = gen_header_dict(header_data)
        #self.print_header(self.info)
        self.rtc_usec = self.info["rtc_usec"]
        self.boot_time_stamp = self.info["stamp"]
        self.boot_time = self.info["system_time"]
        self.records = record_list(self.rtc_usec)

    def read_records(self, f, count=0, rfilter=None, need_done=False):
        record_size = calcsize(record_format)
        record_data = f.read(record_size)
        c = 0
        while record_data != "" :
            c = c + 1
            if (count != 0) and (c > count):
                break
            if len(record_data) != record_size:
                print "Error: len: %u" % len(record_data)
                break
            record = filter_record(record_data, rfilter)
            self.records.push(record, need_done)
            record_data = f.read(record_size)

    def get_records(self, calc_time=False):
        r = self.records.get_info()[0]
        if calc_time:
            self.calc_record_time(r)
        return r

    def get_objects(self):
        return self.records.get_info()[1]

    def get_statics(self):
        return self.records.get_statics()

    def load(self, count=0):
        f = open(self._file_name, "rb")
        self.read_header(f)
        self.read_records(f, count)
        f.close()

if __name__ == "__main__":
    import time

    def my_filter(rba):
        tid = rba[2]
        name = get_rba_traffic_id_name(tid)
        if name == "LUN" or name == "SPC" or name == "PDO" or name == "RG" or name == "MLU":
            return rba
        else:
            return None

    def print_time(info, t):
        gm = t[0]
        ms = t[1]
        print "%s: %u/%02u/%02u %02u:%02u:%02u.%03u" % \
            (info, gm.tm_year, gm.tm_mon, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec, int(ms))

    if len(sys.argv) < 2:
        print "Usage %s file" % sys.argv[0]
        sys.exit(1)

    start = time.clock()
    rf = rba_file()
    f = open(sys.argv[1], "rb")
    rf.read_header(f)
    rf.read_records(f)
    end = time.clock()
    print "It took %.3fms" % ((end - start) * 1000)

    objects = rf.get_objects()
    records = rf.get_records()
    boot = to_utc_time(rf.boot_time)
    print "\n"
    print_time("BOOT", boot)
    if records:
        first_record_time = stamp_to_utc(records[0]._ts, rf.boot_time, rf.boot_time_stamp, rf.rtc_usec)
        print_time("DATE", first_record_time)

    for _,o in objects.items():
        print o

    a = range(100, 120)
    print
    print "Record [%u .. %u]" % (a[0], a[-1])
    for r in a:
        print rf.get_record_desc(records[r])

    print rf.get_statics()


