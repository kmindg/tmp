#!/usr/bin/python

import time
import math
from struct import *

rba_id_array = (
    "",  # KT_NULL
    "",  # KT_TIME
    "",  # KT_LOST
    "",  # KT_FILE_OPEN_WRITE
    "",  # KT_FILE_OPEN_READ
    "",  # KT_FILE_CLOSE
    "",  # KT_FILE_CLOSED
    "TCD",
    "PSM",
    "LUN",
    "FRU",
    "DBE",
    "SFE",
    "DML",
    "MVS",
    "MVA",
    "MLU",
    "SNAP",
    "AGG",
    "MIG",
    "CLONE",
    "CPM",
    "FEDISK",
    "COMPRESSION",
    "FBE",
    "FCT",
    "LUN",
    "FRU",
    "RG",
    "0x1d",
    "0x1e",
    "0x1f",
    "LDO",
    "PDO",
    "PORT",
    "VD",
    "PVD",
    "DDS",
    "SPC",
    "CBFS",
    "CCB",
);

class RBA_traffic_type:
    KT_NULL = 0x00
    KT_TIME = 0x01
    KT_LOST = 0x02
    KT_FILE_OPEN_WRITE = 0x03
    KT_FILE_OPEN_READ = 0x04
    KT_FILE_CLOSE = 0x05
    KT_FILE_CLOSED = 0x06
    KT_TCD_TRAFFIC = 0x07
    KT_PSM_TRAFFIC = 0x08
    KT_LUN_TRAFFIC = 0x09
    KT_FRU_TRAFFIC = 0x0a
    KT_DBE_TRAFFIC = 0x0b
    KT_SFE_TRAFFIC = 0x0c
    KT_DML_TRAFFIC = 0x0d
    KT_MVS_TRAFFIC = 0x0e
    KT_MVA_TRAFFIC = 0x0f
    KT_MLU_TRAFFIC = 0x10
    KT_SNAP_TRAFFIC = 0x11
    KT_AGG_TRAFFIC = 0x12
    KT_MIG_TRAFFIC = 0x13
    KT_CLONES_TRAFFIC = 0x14
    KT_CPM_TRAFFIC = 0x15
    KT_FEDISK_TRAFFIC = 0x16
    KT_COMPRESSION_TRAFFIC = 0x17
    KT_FBE_TRAFFIC = 0x18
    KT_FCT_TRAFFIC = 0x19
    KT_FBE_LUN_TRAFFIC = 0x1a
    KT_FBE_RG_FRU_TRAFFIC = 0x1b
    KT_FBE_RG_TRAFFIC = 0x1c
    KT_0x1d = 0x1d
    KT_0x1e = 0x1e
    KT_0x1f = 0x1f
    KT_FBE_LDO_TRAFFIC = 0x20
    KT_FBE_PDO_TRAFFIC = 0x21
    KT_FBE_PORT_TRAFFIC = 0x22
    KT_FBE_VD_TRAFFIC = 0x23
    KT_FBE_PVD_TRAFFIC = 0x24
    KT_DDS_TRAFFIC = 0x25
    KT_SPC_TRAFFIC = 0x26
    KT_CBFS_TRAFFIC = 0x27
    KT_CCB_TRAFFIC = 0x28
    KT_LAST = KT_CCB_TRAFFIC


cmd_dict = {
 0x0000 : 'READ' ,
 0x0002 : 'WRITE',
 0x0004 : 'ZERO' ,
 0x0006 : 'CRPT_CRC',
 0x0008 : 'MOVE',
 0x000a : 'XCHANGE',
 0x000c : 'COMMIT',
 0x000e : 'NO_OP'

#define KT_GET_MDR      0x0008
#define KT_COPY         0x000a
#define KT_PASSTHRU     0x000c
}
priority_dict = {
 'LOW' : 1,
 'NORMAL' : 2,
 'URGENT' : 3,
}
def get_priority_key_by_value(val):
    return [k for k, v in priority_dict.iteritems() if v == val][0]


rba_header_desc = ("magic", "major", "minor", "ring_id", "reserved0", "head_size", "data_size",
                   "ring_size", "string_size", "elements", "rtc_freq", "stamp", "system_time",
                   "name_ofst", "text_ofst", "base_addr", "wrap_pos", "relative_time")

rba_header_format = "8sBBBBQQQQBqqqHHBqq"
record_format = "QQQQQQQ"

RBA_XSEC = 10.0 * 1000 * 1000 # 100ns

def get_rba_traffic_id_name(tid):
    if tid < 0 or tid >= len(rba_id_array):
        return ""
    else:
        return rba_id_array[tid]

def get_rba_traffic_cmd_name(cmd):
    cmd = cmd & 0xe
    return cmd_dict[cmd]

def gen_header(data):
    header_size = calcsize(rba_header_format)
    #print "Header size:", header_size
    info = unpack(rba_header_format, data[:104])
    return zip(rba_header_desc, info)

def gen_header_dict(data):
    info = dict(gen_header(data))
    info["rtc_usec"] = info["rtc_freq"] / 1000000.0
    return info

def stamp_to_xsec(stamp, rtc_usec):
    # xsec is 100ns
    xsec =  (stamp / rtc_usec) * 10
    return xsec

def stamp_to_ms(stamp, rtc_usec):
    ms = (stamp/ rtc_usec) / 1000.0
    return ms

def get_record_time(rba, boot_time, boot_time_stamp, rtc_usec):
    ts, tr, oid, a0, a1, a2, a3 = unpack(record_format, rba)
    cur_time = boot_time + stamp_to_xsec(ts - boot_time_stamp, rtc_usec)
    return cur_time

def get_record_time_by_header(header, rba):
    boot_time = header["system_time"]
    boot_time_stamp = header["stamp"]
    rtc_usec = header["rtc_usec"]
    return get_record_time(rba, boot_time, boot_time_stamp, rtc_usec)

def get_rba_time_desc(time_info):
    gm = time_info[0]
    ms = time_info[1]
    return "%u %02u:%02u:%02u.%03u" % (gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec, int(ms))

def to_utc_time(t):
    START_OF_EPOCH = 0x19db1ded53e8000 # start time from Jan 1, 1970
    if (t < START_OF_EPOCH):
        t = 0
    else:
        t -= START_OF_EPOCH
    t /= RBA_XSEC
    ms = (t - math.floor(t)) * 1000
    return (time.gmtime(t), ms)

def stamp_to_abs_xsec(stamp, boot_time, boot_time_stamp, rtc_usec):
    return boot_time + stamp_to_xsec(stamp - boot_time_stamp, rtc_usec)

def stamp_to_utc(stamp, boot_time, boot_time_stamp, rtc_usec):
    cur_time = boot_time + stamp_to_xsec(stamp - boot_time_stamp, rtc_usec)
    return to_utc_time(cur_time)

if __name__ == "__main__":
    t, ms = to_utc_time(130184633294000068) #2013/07/16 15:48:49.400
    print t, ms
    print RBA_traffic_type.KT_LUN_TRAFFIC
